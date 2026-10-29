"""Run the shell relay inside ConPTY to test actual console handles and raw input."""

import argparse
import ctypes
from ctypes import wintypes
import msvcrt
import os
import socket
import subprocess
import sys
import threading
import time

from test_shell_relay import receive_events, STARTED, INPUT, STDOUT, STDERR, EXITED


TIMEOUT = 30
PSEUDOCONSOLE_ATTRIBUTE = 0x00020016
EXTENDED_STARTUPINFO_PRESENT = 0x00080000
STARTF_USESTDHANDLES = 0x00000100
kernel = ctypes.WinDLL("kernel32", use_last_error=True)


class Coord(ctypes.Structure):
    _fields_ = [("X", ctypes.c_short), ("Y", ctypes.c_short)]


class StartupInfo(ctypes.Structure):
    _fields_ = [
        ("cb", wintypes.DWORD), ("reserved", wintypes.LPWSTR),
        ("desktop", wintypes.LPWSTR), ("title", wintypes.LPWSTR),
        ("x", wintypes.DWORD), ("y", wintypes.DWORD),
        ("width", wintypes.DWORD), ("height", wintypes.DWORD),
        ("chars_x", wintypes.DWORD), ("chars_y", wintypes.DWORD),
        ("fill", wintypes.DWORD), ("flags", wintypes.DWORD),
        ("show", wintypes.WORD), ("reserved_size", wintypes.WORD),
        ("reserved_bytes", ctypes.c_void_p), ("stdin", wintypes.HANDLE),
        ("stdout", wintypes.HANDLE), ("stderr", wintypes.HANDLE),
        ("attributes", ctypes.c_void_p),
    ]


class ProcessInfo(ctypes.Structure):
    _fields_ = [("process", wintypes.HANDLE), ("thread", wintypes.HANDLE),
                ("pid", wintypes.DWORD), ("tid", wintypes.DWORD)]


def bind(name, result, *args):
    function = getattr(kernel, name)
    function.restype = result
    function.argtypes = args
    return function


close = bind("CloseHandle", wintypes.BOOL, wintypes.HANDLE)
pipe = bind("CreatePipe", wintypes.BOOL, ctypes.POINTER(wintypes.HANDLE),
            ctypes.POINTER(wintypes.HANDLE), ctypes.c_void_p, wintypes.DWORD)
create_console = bind("CreatePseudoConsole", ctypes.c_long, Coord, wintypes.HANDLE,
                      wintypes.HANDLE, wintypes.DWORD, ctypes.POINTER(wintypes.HANDLE))
close_console = bind("ClosePseudoConsole", None, wintypes.HANDLE)
resize = bind("ResizePseudoConsole", ctypes.c_long, wintypes.HANDLE, Coord)
init_attributes = bind("InitializeProcThreadAttributeList", wintypes.BOOL,
                       ctypes.c_void_p, wintypes.DWORD, wintypes.DWORD,
                       ctypes.POINTER(ctypes.c_size_t))
update_attributes = bind("UpdateProcThreadAttribute", wintypes.BOOL, ctypes.c_void_p,
                         wintypes.DWORD, ctypes.c_size_t, ctypes.c_void_p,
                         ctypes.c_size_t, ctypes.c_void_p, ctypes.c_void_p)
delete_attributes = bind("DeleteProcThreadAttributeList", None, ctypes.c_void_p)
create_process = bind("CreateProcessW", wintypes.BOOL, wintypes.LPCWSTR, wintypes.LPWSTR,
                      ctypes.c_void_p, ctypes.c_void_p, wintypes.BOOL, wintypes.DWORD,
                      ctypes.c_void_p, wintypes.LPCWSTR, ctypes.POINTER(StartupInfo),
                      ctypes.POINTER(ProcessInfo))
wait = bind("WaitForSingleObject", wintypes.DWORD, wintypes.HANDLE, wintypes.DWORD)
exit_code = bind("GetExitCodeProcess", wintypes.BOOL, wintypes.HANDLE,
                 ctypes.POINTER(wintypes.DWORD))
terminate = bind("TerminateProcess", wintypes.BOOL, wintypes.HANDLE, wintypes.UINT)


def checked(success):
    if not success:
        raise ctypes.WinError(ctypes.get_last_error())


class Terminal:
    def __init__(self, command):
        self.handles = []
        self.console = wintypes.HANDLE()
        self.process = ProcessInfo()
        self.input = self.output = None
        self.reader = None
        self.bytes = bytearray()
        self.condition = threading.Condition()
        self.reader_error = None
        try:
            child_input, parent_input = self.make_pipe()
            parent_output, child_output = self.make_pipe()
            checked(create_console(Coord(180, 40), child_input, child_output, 0,
                                   ctypes.byref(self.console)) == 0)
            size = ctypes.c_size_t()
            init_attributes(None, 1, 0, ctypes.byref(size))
            storage = ctypes.create_string_buffer(size.value)
            checked(init_attributes(storage, 1, 0, ctypes.byref(size)))
            try:
                checked(update_attributes(storage, 0, PSEUDOCONSOLE_ATTRIBUTE, self.console,
                                          ctypes.sizeof(self.console), None, None))
                startup = StartupInfo()
                startup.cb = ctypes.sizeof(startup)
                startup.flags = STARTF_USESTDHANDLES  # Suppress inheritance of the harness's std handles.
                startup.attributes = ctypes.addressof(storage)
                checked(create_process(None, ctypes.create_unicode_buffer(command), None,
                                       None, False, EXTENDED_STARTUPINFO_PRESENT, None, None,
                                       ctypes.byref(startup), ctypes.byref(self.process)))
            finally:
                delete_attributes(storage)
            for handle in (child_input, child_output):
                close(handle)
                self.handles.remove(handle)
            self.input = msvcrt.open_osfhandle(parent_input.value, os.O_BINARY)
            self.handles.remove(parent_input)
            self.output = msvcrt.open_osfhandle(parent_output.value, os.O_BINARY)
            self.handles.remove(parent_output)
            self.reader = threading.Thread(target=self.read, daemon=True)
            self.reader.start()
        except BaseException:
            self.close()
            raise

    def make_pipe(self):
        read, write = wintypes.HANDLE(), wintypes.HANDLE()
        checked(pipe(ctypes.byref(read), ctypes.byref(write), None, 0))
        self.handles.extend((read, write))
        return read, write

    def read(self):
        try:
            while True:
                data = os.read(self.output, 16384)
                if not data:
                    return
                with self.condition:
                    self.bytes.extend(data)
                    self.condition.notify_all()
        except OSError as error:
            # Broken pipe is normal after the pseudo console closes.
            if error.errno != 22:
                self.reader_error = error

    def send(self, data):
        os.write(self.input, data)

    def expect(self, marker):
        deadline = time.monotonic() + TIMEOUT
        with self.condition:
            while marker not in self.bytes:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise RuntimeError(f"Missing {marker!r}: {bytes(self.bytes[-4096:])!r}")
                self.condition.wait(remaining)

    def close(self):
        if self.process.process:
            if wait(self.process.process, 0) != 0:
                terminate(self.process.process, 1)
                wait(self.process.process, TIMEOUT * 1000)
            close(self.process.process)
            close(self.process.thread)
        if self.console:
            close_console(self.console)
        if self.reader is not None:
            self.reader.join(TIMEOUT)
        for descriptor in (self.input, self.output):
            if descriptor is not None:
                os.close(descriptor)
        for handle in self.handles:
            close(handle)


def test_terminal(executable):
    events, errors = [], []
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(TIMEOUT)
        receiver = threading.Thread(target=receive_events, args=(listener, events, errors))
        receiver.start()
        terminal = None
        try:
            terminal = Terminal(subprocess.list2cmdline(
                [executable, "--port", str(listener.getsockname()[1])]))
            terminal.expect(b">")
            python = '"' + sys.executable + '" -c '
            terminal.send((python + '"import os; print(\'TTY-\'+str(all(os.isatty(n) '
                           'for n in range(3))))"\r').encode())
            # This output must come from execution, not the echoed command text.
            terminal.expect(b"TTY-True")
            terminal.send((python + '"import msvcrt; print(\'KEY-\'+\'READY\', flush=True); '
                           'print(\'KEY-\'+str(ord(msvcrt.getwch())))"\r').encode())
            terminal.expect(b"KEY-READY")
            terminal.send(b"x")
            terminal.expect(b"KEY-120")
            checked(resize(terminal.console, Coord(120, 32)) == 0)
            time.sleep(0.1)
            terminal.send((python + '"import os; print(os.get_terminal_size())"\r').encode())
            terminal.expect(b"columns=120, lines=32")
            terminal.send((python + '"import time; print(\'WAIT-\'+\'READY\', flush=True); '
                           'time.sleep(30)"\r').encode())
            terminal.expect(b"WAIT-READY")
            terminal.send(b"\x03")
            terminal.send((python + '"print(\'AFTER-\'+\'INTERRUPT\')"\r').encode())
            terminal.expect(b"AFTER-INTERRUPT")
            terminal.send(b"exit 7\r")
            checked(wait(terminal.process.process, TIMEOUT * 1000) == 0)
            code = wintypes.DWORD()
            checked(exit_code(terminal.process.process, ctypes.byref(code)))
            if code.value != 7:
                raise RuntimeError(f"Exit status {code.value}: {bytes(terminal.bytes)!r}")
        finally:
            if terminal is not None:
                terminal.close()
            receiver.join(TIMEOUT + 1)
    if receiver.is_alive() or errors:
        raise RuntimeError(f"Notification receiver failed: {errors}")
    if terminal.reader.is_alive() or terminal.reader_error is not None:
        raise RuntimeError(f"Terminal output reader failed: {terminal.reader_error}")
    if events[0] != (STARTED, b"cmd.exe") or events[-1] != (EXITED, b"7"):
        raise RuntimeError(f"Unexpected lifecycle: {events}")
    output = b"".join(data for kind, data in events if kind == STDOUT)
    if b"TTY-True" not in output or b"KEY-120" not in output:
        raise RuntimeError("Terminal output notifications are missing")
    # ConPTY may encode keys using Win32 input mode rather than literal characters.
    if not any(kind == INPUT and data for kind, data in events):
        raise RuntimeError("Raw input notification is missing")


def test_pipe(executable):
    events, errors = [], []
    commands = b"echo relay-out\r\necho relay-err >&2\r\nexit 7\r\n"
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(TIMEOUT)
        receiver = threading.Thread(target=receive_events, args=(listener, events, errors))
        receiver.start()
        try:
            result = subprocess.run(
                [executable, "--port", str(listener.getsockname()[1])], input=commands,
                capture_output=True, timeout=TIMEOUT, check=False)
        finally:
            receiver.join(TIMEOUT + 1)
    if receiver.is_alive() or errors:
        raise RuntimeError(f"Pipe notification receiver failed: {errors}")
    if result.returncode != 7 or b"relay-out" not in result.stdout or b"relay-err" not in result.stderr:
        raise RuntimeError(f"Pipe result mismatch: {result}")
    if events[0] != (STARTED, b"cmd.exe") or events[-1] != (EXITED, b"7"):
        raise RuntimeError(f"Pipe lifecycle mismatch: {events}")
    for kind, expected in ((INPUT, commands), (STDOUT, result.stdout), (STDERR, result.stderr)):
        if b"".join(data for event_kind, data in events if event_kind == kind) != expected:
            raise RuntimeError(f"Pipe notification {kind} does not match the relayed bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable")
    executable = parser.parse_args().executable
    test_pipe(executable)
    test_terminal(executable)
    print("Windows relay: pipes, console handles, raw input, resize, interrupt and exit passed")

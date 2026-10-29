"""Exercise the Unix shell relay and its notification protocol without launching a renderer."""

import argparse
import socket
import struct
import subprocess
import threading
import time


TIMEOUT_SECONDS = 15
HEADER = struct.Struct("!4sBI")
STARTED, INPUT, STDOUT, STDERR, EXITED = range(1, 6)


def receive_events(listener, events, errors):
    try:
        connection, _ = listener.accept()
        with connection:
            connection.settimeout(TIMEOUT_SECONDS)
            pending = b""
            while True:
                chunk = connection.recv(16384)
                if not chunk:
                    if pending:
                        raise ValueError("Truncated notification")
                    return
                pending += chunk
                while len(pending) >= HEADER.size:
                    magic, kind, size = HEADER.unpack_from(pending)
                    if magic != b"ESH1" or kind not in range(STARTED, EXITED + 1) or size > 16384:
                        raise ValueError("Invalid notification header")
                    if len(pending) < HEADER.size + size:
                        break
                    events.append((kind, pending[HEADER.size:HEADER.size + size]))
                    pending = pending[HEADER.size + size:]
    except Exception as error:
        errors.append(error)


def test_relay(executable):
    events, errors = [], []
    commands = b"printf 'relay-out'; printf 'relay-err' >&2; exit 7\n"
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(TIMEOUT_SECONDS)
        receiver = threading.Thread(target=receive_events, args=(listener, events, errors))
        receiver.start()
        try:
            result = subprocess.run(
                [executable, "--port", str(listener.getsockname()[1])], input=commands,
                capture_output=True, timeout=TIMEOUT_SECONDS, check=False)
        finally:
            receiver.join(TIMEOUT_SECONDS + 1)
    if receiver.is_alive() or errors:
        raise RuntimeError(f"Notification receiver failed: {errors}")
    if result.returncode != 7 or result.stdout != b"relay-out" or result.stderr != b"relay-err":
        raise RuntimeError(f"Shell result mismatch: {result}")
    if not events or events[0] != (STARTED, b"/bin/sh") or events[-1] != (EXITED, b"7"):
        raise RuntimeError(f"Lifecycle notifications mismatch: {events}")
    for kind, expected in ((INPUT, commands), (STDOUT, result.stdout), (STDERR, result.stderr)):
        actual = b"".join(data for event_kind, data in events if event_kind == kind)
        if actual != expected:
            raise RuntimeError(f"Notification {kind}: expected {expected!r}, got {actual!r}")


def test_terminal(executable):
    import fcntl
    import os
    import pty
    import select
    import termios

    master, slave = pty.openpty()
    original = termios.tcgetattr(slave)
    events, errors = [], []
    output = bytearray()
    sent = bytearray()

    def send(data):
        sent.extend(data)
        os.write(master, data)

    def expect(marker):
        deadline = time.monotonic() + TIMEOUT_SECONDS
        while marker not in output:
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not select.select([master], [], [], remaining)[0]:
                raise RuntimeError(f"Missing {marker!r}; terminal output: {bytes(output)!r}")
            output.extend(os.read(master, 16384))

    process = None
    try:
        fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 100, 0, 0))
        with socket.socket() as listener:
            listener.bind(("127.0.0.1", 0))
            listener.listen(1)
            listener.settimeout(TIMEOUT_SECONDS)
            receiver = threading.Thread(target=receive_events, args=(listener, events, errors))
            receiver.start()
            try:
                process = subprocess.Popen(
                    [executable, "--port", str(listener.getsockname()[1])],
                    stdin=slave, stdout=slave, stderr=slave,
                    env={**os.environ, "TERM": "xterm-256color", "PS1": "relay-ready> "})
                expect(b"relay-ready> ")
                # The marker is split in the command so terminal echo cannot satisfy the check.
                send(b"test -t 0 && test -t 1 && test -t 2 && test -r /dev/tty && printf 'TTY-%s\\n' OK\n")
                expect(b"TTY-OK\r\n")
                send(b"stty size\n")
                expect(b"24 100\r\n")
                fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 37, 113, 0, 0))
                # Let the relay's size poll run before requesting the child's dimensions.
                time.sleep(0.1)
                send(b"stty size\n")
                expect(b"37 113\r\n")
                send(b"printf 'WAIT-%s\\n' READY; sleep 30\n")
                expect(b"WAIT-READY\r\n")
                time.sleep(0.1)
                send(b"\x03")
                send(b"printf 'AFTER-%s\\n' INTERRUPT\n")
                expect(b"AFTER-INTERRUPT\r\n")
                send(b"exit 7\n")
                if process.wait(timeout=TIMEOUT_SECONDS) != 7:
                    raise RuntimeError(f"Terminal exit status: {process.returncode}")
                if termios.tcgetattr(slave) != original:
                    raise RuntimeError("Relay did not restore the terminal mode")
            finally:
                if process is not None and process.poll() is None:
                    process.kill()
                    process.wait(timeout=TIMEOUT_SECONDS)
                receiver.join(TIMEOUT_SECONDS + 1)
        if receiver.is_alive() or errors:
            raise RuntimeError(f"Terminal notification receiver failed: {errors}")
        if events[0] != (STARTED, b"/bin/sh") or events[-1] != (EXITED, b"7"):
            raise RuntimeError(f"Terminal lifecycle mismatch: {events}")
        notified_input = b"".join(data for kind, data in events if kind == INPUT)
        notified_output = b"".join(data for kind, data in events if kind == STDOUT)
        if notified_input != sent or not notified_output.startswith(output):
            raise RuntimeError("Terminal I/O notifications do not match relayed bytes")
        if any(kind == STDERR for kind, _ in events):
            raise RuntimeError("PTY output should be a single merged stream")
    finally:
        os.close(master)
        os.close(slave)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable")
    args = parser.parse_args()
    test_relay(args.executable)
    test_terminal(args.executable)
    print("Shell relay: command, stdout, stderr, exit status and notifications passed")


if __name__ == "__main__":
    main()

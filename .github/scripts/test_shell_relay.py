"""Exercise the Unix shell relay and its notification protocol without launching a renderer."""

import argparse
import socket
import struct
import subprocess
import threading


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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable")
    args = parser.parse_args()
    test_relay(args.executable)
    print("Shell relay: command, stdout, stderr, exit status and notifications passed")


if __name__ == "__main__":
    main()

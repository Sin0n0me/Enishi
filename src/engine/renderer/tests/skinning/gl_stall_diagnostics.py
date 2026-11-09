"""Bounded native stack capture for stalled macOS graphics tests."""
from contextlib import contextmanager
import os
from pathlib import Path
import subprocess
import tempfile
import threading

SAMPLE_DURATION_SECONDS = 1
SAMPLE_INTERVAL_MILLISECONDS = 10
SAMPLE_TIMEOUT_SECONDS = 10


def capture_native_stack():
    print("[OpenGL] begin native stack sample", flush=True)
    try:
        with tempfile.TemporaryDirectory(prefix="enishi-gl-sample-") as directory:
            report = Path(directory) / "sample.txt"
            result = subprocess.run(
                ["/usr/bin/sample", str(os.getpid()), str(SAMPLE_DURATION_SECONDS),
                 str(SAMPLE_INTERVAL_MILLISECONDS), "-file", str(report)],
                capture_output=True, text=True, encoding="utf-8", errors="replace",
                timeout=SAMPLE_TIMEOUT_SECONDS, check=False)
            print(f"[OpenGL] sample exit code: {result.returncode}", flush=True)
            for output in (result.stdout, result.stderr):
                if output:
                    print(output, flush=True)
            if report.is_file():
                print(report.read_text(encoding="utf-8", errors="replace"), flush=True)
            else:
                print("[OpenGL] sample produced no stack report", flush=True)
    except (OSError, subprocess.SubprocessError) as error:
        # Diagnostics must not replace the assertion or timeout being investigated.
        print(f"[OpenGL] native stack capture failed: {error}", flush=True)
    finally:
        print("[OpenGL] end native stack sample", flush=True)


@contextmanager
def native_stall_watchdog(delay_seconds):
    timer = threading.Timer(delay_seconds, capture_native_stack)
    timer.daemon = True
    timer.start()
    try:
        yield
    finally:
        timer.cancel()
        # A sample already in progress has its own subprocess timeout. Do not let
        # slow diagnostic cleanup keep a failed graphics test alive indefinitely.
        timer.join(timeout=SAMPLE_TIMEOUT_SECONDS)

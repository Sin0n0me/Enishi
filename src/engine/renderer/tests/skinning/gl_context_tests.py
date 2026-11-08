"""Exercise context cleanup on both successful and failed test callbacks."""
import ctypes as c
from contextlib import redirect_stdout
import io
import os
from pathlib import Path
import subprocess
import unittest
from unittest.mock import patch
from gl_test_context import gl, run_with_context
import gl_stall_diagnostics as diagnostics

GL_VERSION = 0x1F02
REPEATS = 3


class ContextTests(unittest.TestCase):
    def assert_current_context(self):
        version = gl("glGetString", c.c_char_p, c.c_uint)(GL_VERSION)
        self.assertIsNotNone(version)

    def test_repeated_context_creation(self):
        for iteration in range(REPEATS):
            with self.subTest(iteration=iteration):
                run_with_context(self.assert_current_context)

    def test_callback_failure_propagates_and_next_context_works(self):
        def fail():
            self.assert_current_context()
            raise RuntimeError("injected graphics assertion failure")

        for iteration in range(REPEATS):
            with self.subTest(iteration=iteration):
                with self.assertRaisesRegex(RuntimeError, "injected graphics assertion failure"):
                    run_with_context(fail)
                run_with_context(self.assert_current_context)


class NativeDiagnosticTests(unittest.TestCase):
    def test_sample_reports_native_stack_and_cleans_temporary_file(self):
        reports = []

        def sample(command, **options):
            self.assertEqual(command[:4], ["/usr/bin/sample", str(os.getpid()), "1", "10"])
            self.assertEqual(command[4], "-file")
            self.assertEqual(options["timeout"], diagnostics.SAMPLE_TIMEOUT_SECONDS)
            self.assertFalse(options["check"])
            report = Path(command[5])
            reports.append(report)
            report.write_text("driver_native_frame", encoding="utf-8")
            return subprocess.CompletedProcess(command, 0, "sample stdout", "sample stderr")

        output = io.StringIO()
        with patch.object(diagnostics.subprocess, "run", side_effect=sample), redirect_stdout(output):
            diagnostics.capture_native_stack()
        for expected in ("driver_native_frame", "sample stdout", "sample stderr", "exit code: 0"):
            self.assertIn(expected, output.getvalue())
        self.assertEqual(len(reports), 1)
        self.assertFalse(reports[0].parent.exists())

    def test_sample_failure_is_visible_without_raising(self):
        failures = (FileNotFoundError("sample unavailable"), PermissionError("sampling denied"),
                    subprocess.TimeoutExpired("sample", diagnostics.SAMPLE_TIMEOUT_SECONDS))
        for failure in failures:
            with self.subTest(failure=failure):
                output = io.StringIO()
                with patch.object(diagnostics.subprocess, "run", side_effect=failure), redirect_stdout(output):
                    diagnostics.capture_native_stack()
                self.assertIn(str(failure), output.getvalue())
                self.assertIn("end native stack sample", output.getvalue())

    def test_sample_nonzero_exit_and_missing_report_are_visible(self):
        result = subprocess.CompletedProcess([], 1, "", "not permitted")
        output = io.StringIO()
        with patch.object(diagnostics.subprocess, "run", return_value=result), redirect_stdout(output):
            diagnostics.capture_native_stack()
        for expected in ("exit code: 1", "not permitted", "no stack report"):
            self.assertIn(expected, output.getvalue())

    def test_watchdog_cancels_on_success_and_preserves_failure(self):
        for fails in (False, True):
            with self.subTest(fails=fails), patch.object(diagnostics.threading, "Timer") as timer_type:
                failure = RuntimeError("original graphics failure")
                try:
                    with diagnostics.native_stall_watchdog(30):
                        timer_type.assert_called_once_with(30, diagnostics.capture_native_stack)
                        timer_type.return_value.start.assert_called_once_with()
                        if fails:
                            raise failure
                except RuntimeError as error:
                    self.assertIs(error, failure)
                else:
                    self.assertFalse(fails)
                timer = timer_type.return_value
                self.assertTrue(timer.daemon)
                timer.cancel.assert_called_once_with()
                timer.join.assert_called_once_with(timeout=diagnostics.SAMPLE_TIMEOUT_SECONDS)


if __name__ == "__main__":
    unittest.main()

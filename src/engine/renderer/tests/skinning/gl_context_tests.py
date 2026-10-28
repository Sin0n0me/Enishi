"""Exercise context cleanup on both successful and failed test callbacks."""
import ctypes as c
import unittest
from gl_test_context import gl, run_with_context

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


if __name__ == "__main__":
    unittest.main()

"""Select an OS context without coupling shader assertions to a window system."""
import sys

if sys.platform == "win32":
    from gl_context_wgl import gl, run_with_context
elif sys.platform.startswith("linux"):
    from gl_context_egl import gl, run_with_context
elif sys.platform == "darwin":
    from gl_context_cgl import gl, run_with_context
else:
    raise RuntimeError(f"OpenGL test context is not implemented for {sys.platform}; "
                       "add a platform context provider before enabling these tests")

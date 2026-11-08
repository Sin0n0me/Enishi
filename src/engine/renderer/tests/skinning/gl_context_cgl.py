"""macOS offscreen OpenGL 4.1 core context for the shared rendering tests."""
import ctypes as c
import faulthandler
import sys
import time
from gl_test_checks import check
from gl_stall_diagnostics import native_stall_watchdog

CGL_OPENGL_PROFILE = 99
CGL_OPENGL_41_CORE = 0x4100
CGL_ALLOW_OFFLINE_RENDERERS = 96
STALL_DIAGNOSTIC_SECONDS = 30
GL_VENDOR = 0x1F00
GL_RENDERER = 0x1F01
GL_VERSION = 0x1F02
TRACED_GL_CALLS = frozenset((
    "glCompileShader", "glGetShaderiv", "glLinkProgram", "glGetProgramiv",
    "glDrawArrays", "glReadPixels", "glGetBufferSubData", "glFinish",
))
framework = c.CDLL("/System/Library/Frameworks/OpenGL.framework/OpenGL")


def gl(name, result, *args):
    entry = getattr(framework, name)
    entry.restype = result
    entry.argtypes = args
    if name.startswith("CGL") or name in TRACED_GL_CALLS:
        def traced(*values):
            print(f"[OpenGL] begin {name}", flush=True)
            started = time.monotonic()
            result = entry(*values)
            print(f"[OpenGL] end {name} ({time.monotonic() - started:.3f}s)", flush=True)
            return result
        return traced
    return entry


choose_format = gl("CGLChoosePixelFormat", c.c_int, c.POINTER(c.c_int),
                   c.POINTER(c.c_void_p), c.POINTER(c.c_int))
create_context = gl("CGLCreateContext", c.c_int, c.c_void_p, c.c_void_p,
                    c.POINTER(c.c_void_p))
get_current = gl("CGLGetCurrentContext", c.c_void_p)
set_current = gl("CGLSetCurrentContext", c.c_int, c.c_void_p)
destroy_context = gl("CGLDestroyContext", c.c_int, c.c_void_p)
destroy_format = gl("CGLDestroyPixelFormat", c.c_int, c.c_void_p)


def require(result, operation):
    check(result == 0, f"{operation} failed (CGL error {result})")


def run_with_context(test):
    # Dump the Python call site before CTest terminates a stalled native GL call.
    faulthandler.dump_traceback_later(STALL_DIAGNOSTIC_SECONDS)
    try:
        with native_stall_watchdog(STALL_DIAGNOSTIC_SECONDS):
            run_context(test)
    finally:
        faulthandler.cancel_dump_traceback_later()


def run_context(test):
    attributes = (c.c_int * 4)(CGL_OPENGL_PROFILE, CGL_OPENGL_41_CORE,
                               CGL_ALLOW_OFFLINE_RENDERERS, 0)
    pixel_format = c.c_void_p()
    count = c.c_int()
    context = c.c_void_p()
    previous = get_current()
    try:
        require(choose_format(attributes, c.byref(pixel_format), c.byref(count)),
                "CGLChoosePixelFormat")
        check(pixel_format.value is not None and count.value > 0, "No OpenGL 4.1 core pixel format")
        require(create_context(pixel_format, None, c.byref(context)), "CGLCreateContext")
        require(set_current(context), "CGLSetCurrentContext")
        get_string = gl("glGetString", c.c_char_p, c.c_uint)
        for name, token in (("vendor", GL_VENDOR), ("renderer", GL_RENDERER), ("version", GL_VERSION)):
            value = get_string(token)
            check(value is not None, f"Missing OpenGL {name}")
            print(f"[OpenGL] {name}: {value.decode('utf-8', errors='replace')}", flush=True)
        print("[OpenGL] begin test callback", flush=True)
        test()
        print("[OpenGL] end test callback", flush=True)
    finally:
        test_failed = sys.exc_info()[0] is not None
        results = [set_current(previous)]
        if context.value is not None:
            results.append(destroy_context(context))
        if pixel_format.value is not None:
            results.append(destroy_format(pixel_format))
        if any(result != 0 for result in results):
            if test_failed:
                print("CGL context cleanup also failed", file=sys.stderr)
            else:
                raise RuntimeError("CGL context cleanup failed")

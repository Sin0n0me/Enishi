"""macOS offscreen OpenGL 4.1 core context for the shared rendering tests."""
import ctypes as c
import sys
from gl_test_checks import check

CGL_OPENGL_PROFILE = 99
CGL_OPENGL_41_CORE = 0x4100
CGL_ALLOW_OFFLINE_RENDERERS = 96
framework = c.CDLL("/System/Library/Frameworks/OpenGL.framework/OpenGL")


def gl(name, result, *args):
    entry = getattr(framework, name)
    entry.restype = result
    entry.argtypes = args
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
        test()
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

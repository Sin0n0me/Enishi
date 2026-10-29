"""Linux headless OpenGL context using the system EGL implementation."""
import ctypes as c
import sys
from gl_test_checks import check

EGL_NONE = 0x3038
EGL_PLATFORM_SURFACELESS_MESA = 0x31DD
EGL_OPENGL_API = 0x30A2
EGL_SURFACE_TYPE = 0x3033
EGL_PBUFFER_BIT = 0x0001
EGL_RENDERABLE_TYPE = 0x3040
EGL_OPENGL_BIT = 0x0008
EGL_RED_SIZE = 0x3024
EGL_GREEN_SIZE = 0x3023
EGL_BLUE_SIZE = 0x3022
EGL_ALPHA_SIZE = 0x3021
EGL_WIDTH = 0x3057
EGL_HEIGHT = 0x3056
EGL_CONTEXT_MAJOR_VERSION = 0x3098
EGL_CONTEXT_MINOR_VERSION = 0x30FB
EGL_CONTEXT_OPENGL_PROFILE_MASK = 0x30FD
EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT = 0x0001
CHANNEL_BITS = 8
SURFACE_SIZE = 16

egl = c.CDLL("libEGL.so.1")


def function(name, result, *args):
    entry = getattr(egl, name)
    entry.restype = result
    entry.argtypes = args
    return entry


get_error = function("eglGetError", c.c_uint)
get_proc = function("eglGetProcAddress", c.c_void_p, c.c_char_p)
get_display = function("eglGetPlatformDisplay", c.c_void_p,
                       c.c_uint, c.c_void_p, c.c_void_p)
initialize = function("eglInitialize", c.c_uint, c.c_void_p,
                      c.POINTER(c.c_int), c.POINTER(c.c_int))
bind_api = function("eglBindAPI", c.c_uint, c.c_uint)
choose_config = function("eglChooseConfig", c.c_uint, c.c_void_p,
                         c.POINTER(c.c_int), c.POINTER(c.c_void_p), c.c_int,
                         c.POINTER(c.c_int))
create_surface = function("eglCreatePbufferSurface", c.c_void_p,
                          c.c_void_p, c.c_void_p, c.POINTER(c.c_int))
create_context = function("eglCreateContext", c.c_void_p,
                          c.c_void_p, c.c_void_p, c.c_void_p, c.POINTER(c.c_int))
make_current = function("eglMakeCurrent", c.c_uint,
                        c.c_void_p, c.c_void_p, c.c_void_p, c.c_void_p)
destroy_context = function("eglDestroyContext", c.c_uint, c.c_void_p, c.c_void_p)
destroy_surface = function("eglDestroySurface", c.c_uint, c.c_void_p, c.c_void_p)
terminate = function("eglTerminate", c.c_uint, c.c_void_p)


def require(value, operation):
    if not value:
        raise RuntimeError(f"{operation} failed (EGL error 0x{get_error():04x})")


def gl(name, result, *args):
    address = get_proc(name.encode("ascii"))
    check(address is not None, f"OpenGL function unavailable: {name}")
    return c.CFUNCTYPE(result, *args)(address)


def run_with_context(test):
    display = get_display(EGL_PLATFORM_SURFACELESS_MESA, None, None)
    require(display, "eglGetPlatformDisplay(surfaceless)")
    require(initialize(display, None, None), "eglInitialize")
    surface = None
    context = None
    try:
        require(bind_api(EGL_OPENGL_API), "eglBindAPI(OpenGL)")
        attributes = (c.c_int * 13)(
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_RED_SIZE, CHANNEL_BITS, EGL_GREEN_SIZE, CHANNEL_BITS,
            EGL_BLUE_SIZE, CHANNEL_BITS, EGL_ALPHA_SIZE, CHANNEL_BITS, EGL_NONE)
        config = c.c_void_p()
        count = c.c_int()
        require(choose_config(display, attributes, c.byref(config), 1, c.byref(count)),
                "eglChooseConfig")
        check(count.value == 1 and config.value is not None, "No RGBA OpenGL EGL config")
        size = (c.c_int * 5)(EGL_WIDTH, SURFACE_SIZE, EGL_HEIGHT, SURFACE_SIZE, EGL_NONE)
        surface = create_surface(display, config, size)
        require(surface, "eglCreatePbufferSurface")
        version = (c.c_int * 7)(EGL_CONTEXT_MAJOR_VERSION, 4,
                               EGL_CONTEXT_MINOR_VERSION, 0,
                               EGL_CONTEXT_OPENGL_PROFILE_MASK,
                               EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT, EGL_NONE)
        context = create_context(display, config, None, version)
        require(context, "eglCreateContext(OpenGL 4.0 core)")
        require(make_current(display, surface, surface, context), "eglMakeCurrent")
        test()
    finally:
        test_failed = sys.exc_info()[0] is not None
        # Attempt every cleanup even when setup or an assertion failed.
        results = [make_current(display, None, None, None)]
        if context is not None:
            results.append(destroy_context(display, context))
        if surface is not None:
            results.append(destroy_surface(display, surface))
        results.append(terminate(display))
        if not all(results):
            if test_failed:
                print("EGL context cleanup also failed", file=sys.stderr)
            else:
                raise RuntimeError("EGL context cleanup failed")

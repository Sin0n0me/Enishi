"""Windows hidden-window context for OpenGL regression tests."""
import ctypes as c
from ctypes import wintypes as w
from gl_test_checks import check


class PixelFormat(c.Structure):
    _fields_ = [("size", w.WORD), ("version", w.WORD), ("flags", w.DWORD),
                ("pixel_type", c.c_byte), ("color_bits", c.c_byte),
                ("channels", c.c_byte * 11), ("depth", c.c_byte),
                ("stencil", c.c_byte), ("aux", c.c_byte), ("layer", c.c_byte),
                ("reserved", c.c_byte), ("layer_mask", w.DWORD),
                ("visible_mask", w.DWORD), ("damage_mask", w.DWORD)]


user = c.WinDLL("user32")
gdi = c.WinDLL("gdi32")
opengl = c.WinDLL("opengl32")
user.CreateWindowExW.argtypes = [w.DWORD, w.LPCWSTR, w.LPCWSTR, w.DWORD,
                               c.c_int, c.c_int, c.c_int, c.c_int,
                               w.HWND, w.HMENU, w.HINSTANCE, c.c_void_p]
user.CreateWindowExW.restype = w.HWND
user.GetDC.argtypes = [w.HWND]
user.GetDC.restype = w.HDC
user.ReleaseDC.argtypes = [w.HWND, w.HDC]
user.DestroyWindow.argtypes = [w.HWND]
gdi.ChoosePixelFormat.argtypes = [w.HDC, c.POINTER(PixelFormat)]
gdi.SetPixelFormat.argtypes = [w.HDC, c.c_int, c.POINTER(PixelFormat)]
opengl.wglCreateContext.argtypes = [w.HDC]
opengl.wglCreateContext.restype = c.c_void_p
opengl.wglMakeCurrent.argtypes = [w.HDC, c.c_void_p]
opengl.wglDeleteContext.argtypes = [c.c_void_p]
opengl.wglGetProcAddress.argtypes = [c.c_char_p]
opengl.wglGetProcAddress.restype = c.c_void_p


def gl(name, result, *args):
    address = opengl.wglGetProcAddress(name.encode())
    if address not in (None, 1, 2, 3, c.c_void_p(-1).value):
        return c.WINFUNCTYPE(result, *args)(address)
    function = getattr(opengl, name)
    function.restype = result
    function.argtypes = args
    return function


def run_with_context(test):
    window = user.CreateWindowExW(0, "STATIC", "", 0, 0, 0, 1, 1, None, None, None, None)
    check(window is not None, "hidden window creation failed")
    dc = user.GetDC(window)
    context = None
    try:
        pixel = PixelFormat()
        pixel.size, pixel.version, pixel.flags, pixel.color_bits, pixel.depth = c.sizeof(pixel), 1, 0x24, 32, 24
        index = gdi.ChoosePixelFormat(dc, c.byref(pixel))
        check(index != 0 and gdi.SetPixelFormat(dc, index, c.byref(pixel)) != 0, "pixel format failed")
        context = opengl.wglCreateContext(dc)
        check(context is not None and opengl.wglMakeCurrent(dc, context) != 0, "WGL context failed")
        test()
    finally:
        opengl.wglMakeCurrent(None, None)
        if context is not None:
            opengl.wglDeleteContext(context)
        user.ReleaseDC(window, dc)
        user.DestroyWindow(window)



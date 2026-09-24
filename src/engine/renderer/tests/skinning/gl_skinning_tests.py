"""Run the production GLSL vertex shader with transform feedback on Windows.

Uses a hidden WGL context and the standard library; no packages are downloaded.
"""
import ctypes as c
from ctypes import wintypes as w
from pathlib import Path
import math
import struct


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


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


def identity():
    return [float(row == column) for column in range(4) for row in range(4)]


def rotation(angle):
    m = identity()
    m[0], m[1], m[4], m[5] = math.cos(angle), math.sin(angle), -math.sin(angle), math.cos(angle)
    return m


def run_tests():
    uint = c.c_uint
    integer = c.c_int
    pointer = c.c_void_p
    create_shader = gl("glCreateShader", uint, uint)
    shader = create_shader(0x8B31)  # GL_VERTEX_SHADER
    source = (Path(__file__).resolve().parents[4] / "shader/glsl/vs_model_gl.glsl").read_bytes()
    source_pointer = c.c_char_p(source)
    gl("glShaderSource", None, uint, integer, c.POINTER(c.c_char_p), pointer)(shader, 1, c.byref(source_pointer), None)
    gl("glCompileShader", None, uint)(shader)
    status = integer()
    gl("glGetShaderiv", None, uint, uint, c.POINTER(integer))(shader, 0x8B81, c.byref(status))
    log = c.create_string_buffer(8192)
    gl("glGetShaderInfoLog", None, uint, integer, pointer, pointer)(shader, len(log), None, log)
    check(status.value == 1, log.value.decode())
    program = gl("glCreateProgram", uint)()
    gl("glAttachShader", None, uint, uint)(program, shader)
    varyings = (c.c_char_p * 2)(b"view_position", b"view_normal")
    gl("glTransformFeedbackVaryings", None, uint, integer, c.POINTER(c.c_char_p), uint)(program, 2, varyings, 0x8C8C)
    gl("glLinkProgram", None, uint)(program)
    gl("glGetProgramiv", None, uint, uint, c.POINTER(integer))(program, 0x8B82, c.byref(status))
    gl("glGetProgramInfoLog", None, uint, integer, pointer, pointer)(program, len(log), None, log)
    check(status.value == 1, log.value.decode())
    gl("glUseProgram", None, uint)(program)
    buffers = (uint * 4)()
    gl("glGenBuffers", None, integer, c.POINTER(uint))(4, buffers)
    bind = gl("glBindBuffer", None, uint, uint)
    upload = gl("glBufferData", None, uint, c.c_ssize_t, pointer, uint)
    base = gl("glBindBufferBase", None, uint, uint, uint)
    for name, slot, index in ((b"Camera", 0, 0), (b"Bones", 1, 1)):
        block = gl("glGetUniformBlockIndex", uint, uint, c.c_char_p)(program, name)
        gl("glUniformBlockBinding", None, uint, uint, uint)(program, block, slot)
        base(0x8A11, slot, buffers[index])
    camera = (c.c_float * 64)(*(identity() * 4))
    bind(0x8A11, buffers[0])
    upload(0x8A11, c.sizeof(camera), camera, 0x88E8)
    vao = uint()
    gl("glGenVertexArrays", None, integer, c.POINTER(uint))(1, c.byref(vao))
    gl("glBindVertexArray", None, uint)(vao)
    bind(0x8892, buffers[2])
    for location, count, offset, is_integer in ((0,3,0,False), (1,3,12,False), (2,2,24,False),
            (3,4,32,True), (4,4,48,False), (5,1,68,True), (6,3,72,False), (7,3,84,False), (8,3,96,False)):
        gl("glEnableVertexAttribArray", None, uint)(location)
        if is_integer:
            gl("glVertexAttribIPointer", None, uint, integer, uint, integer, pointer)(location, count, 0x1405, 108, offset)
        else:
            gl("glVertexAttribPointer", None, uint, integer, uint, c.c_ubyte, integer, pointer)(location, count, 0x1406, 0, 108, offset)
    base(0x8C8E, 0, buffers[3])
    bind(0x8C8E, buffers[3])
    upload(0x8C8E, 24, None, 0x88E8)
    gl("glEnable", None, uint)(0x8C89)  # GL_RASTERIZER_DISCARD

    def deform(method, matrices, weights, center=(0,0,0), a0=(0,0,0), a1=(0,0,0)):
        palette = (c.c_float * (512 * 16))(*(sum(matrices, []) + identity() * (512 - len(matrices))))
        bind(0x8A11, buffers[1])
        upload(0x8A11, c.sizeof(palette), palette, 0x88E8)
        vertex = struct.pack("<3f3f2f4I4ffI9f", 1,0,0, 1,0,0, 0,0, 0,1,2,3,
                             *weights, 0, method, *center, *a0, *a1)
        bind(0x8892, buffers[2])
        upload(0x8892, len(vertex), c.c_char_p(vertex), 0x88E8)
        gl("glBeginTransformFeedback", None, uint)(0)  # GL_POINTS
        gl("glDrawArrays", None, uint, integer, integer)(0, 0, 1)
        gl("glEndTransformFeedback", None)()
        output = (c.c_float * 6)()
        bind(0x8C8E, buffers[3])
        gl("glGetBufferSubData", None, uint, c.c_ssize_t, c.c_ssize_t, pointer)(0x8C8E, 0, 24, output)
        check(gl("glGetError", uint)() == 0, "OpenGL execution error")
        return list(output)

    def close(actual, expected):
        check(all(abs(a-b) < 0.0001 for a,b in zip(actual, expected)), f"{actual} != {expected}")

    for method in range(3):
        close(deform(method, [identity()] * 4, (0.5,0.5,0,0)), [1,0,0,1,0,0])
        result = deform(method, [identity(), rotation(math.pi/2), identity(), identity()], (0.5,0.5,0,0))
        expected = 0.5 if method == 0 else math.sqrt(0.5)
        close(result[:3], [expected,expected,0])
        close(result[3:], [math.sqrt(0.5),math.sqrt(0.5),0])
    translated = [identity() for _ in range(4)]
    for index, matrix in enumerate(translated):
        matrix[12] = index
    for method in (0,1):
        close(deform(method, translated, (0.1,0.2,0.3,0.4))[:3], [3,0,0])
    close(deform(1, [rotation(math.radians(170)),rotation(math.radians(-170))], (0.5,0.5,0,0))[:3], [-1,0,0])
    close(deform(2, [identity(),identity()], (0.25,0.75,0,0), (1,2,3), (4,2,3), (0,2,3))[:3], [1,0,0])
    gl("glDeleteBuffers", None, integer, c.POINTER(uint))(4, buffers)
    gl("glDeleteVertexArrays", None, integer, c.POINTER(uint))(1, c.byref(vao))
    gl("glDeleteProgram", None, uint)(program)
    gl("glDeleteShader", None, uint)(shader)
    print("Production GLSL transform feedback tests passed")


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
    run_tests()
finally:
    opengl.wglMakeCurrent(None, None)
    if context is not None:
        opengl.wglDeleteContext(context)
    user.ReleaseDC(window, dc)
    user.DestroyWindow(window)

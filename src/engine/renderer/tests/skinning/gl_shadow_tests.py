"""Validate generated OpenGL shadow shaders in an asset-free hidden WGL context."""
import ctypes as c
from pathlib import Path
import math
import struct
import sys
from gl_test_checks import check
from gl_test_context import gl, run_with_context
from gl_skinning_tests import identity

U = c.c_uint
I = c.c_int
P = c.c_void_p
SIZE = 64
TEXTURE = 0x0DE1
FRAMEBUFFER = 0x8D40
FLOAT = 0x1406
UBO = 0x8A11
ARRAY = 0x8892
TRIANGLES = 4


def object_id(function):
    value = U()
    gl(function, None, I, c.POINTER(U))(1, c.byref(value))
    return value.value


def program(directory, vertex, fragment):
    result = gl("glCreateProgram", U)()
    for filename, stage in ((vertex, 0x8B31), (fragment, 0x8B30)):
        source = (directory / filename).read_bytes()
        shader = gl("glCreateShader", U, U)(stage)
        text = c.c_char_p(source)
        gl("glShaderSource", None, U, I, c.POINTER(c.c_char_p), P)(shader, 1, c.byref(text), None)
        gl("glCompileShader", None, U)(shader)
        status = I()
        gl("glGetShaderiv", None, U, U, c.POINTER(I))(shader, 0x8B81, c.byref(status))
        log = c.create_string_buffer(8192)
        gl("glGetShaderInfoLog", None, U, I, P, P)(shader, len(log), None, log)
        check(status.value == 1, filename + ": " + log.value.decode())
        gl("glAttachShader", None, U, U)(result, shader)
        gl("glDeleteShader", None, U)(shader)
    gl("glLinkProgram", None, U)(result)
    status = I()
    gl("glGetProgramiv", None, U, U, c.POINTER(I))(result, 0x8B82, c.byref(status))
    log = c.create_string_buffer(8192)
    gl("glGetProgramInfoLog", None, U, I, P, P)(result, len(log), None, log)
    check(status.value == 1, log.value.decode())
    for name, slot in ((b"Camera", 0), (b"Bones", 1), (b"ShadowMap", 2), (b"Material", 3)):
        index = gl("glGetUniformBlockIndex", U, U, c.c_char_p)(result, name)
        if index != 0xFFFFFFFF:
            gl("glUniformBlockBinding", None, U, U, U)(result, index, slot)
    gl("glUseProgram", None, U)(result)
    for name, slot in ((b"model_texture", 0), (b"sphere_texture", 1), (b"toon_texture", 2), (b"shadow_texture", 3)):
        location = gl("glGetUniformLocation", I, U, c.c_char_p)(result, name)
        if location != -1:
            gl("glUniform1i", None, I, I)(location, slot)
    return result


def bind_texture(unit, texture):
    gl("glActiveTexture", None, U)(0x84C0 + unit)
    gl("glBindTexture", None, U, U)(TEXTURE, texture)


def texture(internal, fmt, width, height, data=None):
    result = object_id("glGenTextures")
    bind_texture(0, result)
    gl("glTexImage2D", None, U, I, I, I, I, I, U, U, P)(
        TEXTURE, 0, internal, width, height, 0, fmt, FLOAT, data)
    for parameter in (0x2801, 0x2800):  # min/mag filter
        gl("glTexParameteri", None, U, U, I)(TEXTURE, parameter, 0x2600)
    gl("glTexParameteri", None, U, U, I)(TEXTURE, 0x813D, 0)  # max mip level
    return result


def framebuffer(texture_id, attachment, color):
    result = object_id("glGenFramebuffers")
    gl("glBindFramebuffer", None, U, U)(FRAMEBUFFER, result)
    gl("glFramebufferTexture2D", None, U, U, U, U, I)(FRAMEBUFFER, attachment, TEXTURE, texture_id, 0)
    gl("glDrawBuffer", None, U)(0x8CE0 if color else 0)
    gl("glReadBuffer", None, U)(0x8CE0 if color else 0)
    status = gl("glCheckFramebufferStatus", U, U)(FRAMEBUFFER)
    check(status == 0x8CD5, f"incomplete FBO {status:#x}")
    return result


class Runner:
    def __init__(self, directory):
        self.programs = [program(directory, "vs_shadow_map_gl.glsl", "ps_shadow_map_gl.glsl"),
                         program(directory, "vs_model_gl.glsl", "ps_model_gl.glsl"),
                         program(directory, "vs_clear_wall_gl.glsl", "ps_clear_wall_gl.glsl")]
        self.buffers = [object_id("glGenBuffers") for _ in range(5)]
        self.vao = object_id("glGenVertexArrays")
        gl("glBindVertexArray", None, U)(self.vao)
        gl("glBindBuffer", None, U, U)(ARRAY, self.buffers[4])
        for location, count, offset, integer in ((0,3,0,False), (1,3,12,False), (2,2,24,False),
                (3,4,32,True), (4,4,48,False), (5,1,68,True), (6,3,72,False),
                (7,3,84,False), (8,3,96,False)):
            gl("glEnableVertexAttribArray", None, U)(location)
            if integer:
                gl("glVertexAttribIPointer", None, U, I, U, I, P)(location, count, 0x1405, 172, offset)
            else:
                gl("glVertexAttribPointer", None, U, I, U, c.c_ubyte, I, P)(location, count, FLOAT, 0, 172, offset)
        self.uniform(0, identity() * 4)
        self.uniform(2, identity() + [0.0] * 4)
        self.depth = texture(0x8CAC, 0x1902, SIZE, SIZE)  # D32F
        self.shadow_fbo = framebuffer(self.depth, 0x8D00, False)
        self.color = texture(0x8814, 0x1908, SIZE, SIZE)  # RGBA32F
        self.color_fbo = framebuffer(self.color, 0x8CE0, True)
        self.white = texture(0x8814, 0x1908, 1, 1, (c.c_float * 4)(1,1,1,1))
        for unit in range(3):
            bind_texture(unit, self.white)
        gl("glDisable", None, U)(0x0B44)  # culling
        gl("glDisable", None, U)(0x0BE2)  # blending
        gl("glViewport", None, I, I, I, I)(0, 0, SIZE, SIZE)

    def uniform(self, slot, values):
        data = (c.c_float * len(values))(*values)
        gl("glBindBuffer", None, U, U)(UBO, self.buffers[slot])
        gl("glBufferData", None, U, c.c_ssize_t, P, U)(UBO, c.sizeof(data), data, 0x88E8)
        gl("glBindBufferBase", None, U, U, U)(UBO, slot, self.buffers[slot])

    def material(self, alpha):
        values = [0.0] * 48
        values[:4] = [0.6, 0.6, 0.6, alpha]
        values[8:11] = [0.2, 0.2, 0.2]
        for start in (16, 24, 32):
            values[start:start+4] = [1.0] * 4
        self.uniform(3, values)

    def vertices(self, positions):
        normal = (-0.35, 0.8, -1.0)
        norm = math.sqrt(sum(x*x for x in normal))
        normal = [x/norm for x in normal]
        data = b""
        for position in positions:
            data += struct.pack("<3f3f2f4I4ffI9f", *position, *normal, 0,0, 0,0,0,0,
                                1,0,0,0, 0,0, *([0.0]*9))
            data += bytes(64)  # additional UV channels
        gl("glBindBuffer", None, U, U)(ARRAY, self.buffers[4])
        gl("glBufferData", None, U, c.c_ssize_t, P, U)(ARRAY, len(data), c.c_char_p(data), 0x88E8)
        gl("glDrawArrays", None, U, I, I)(TRIANGLES, 0, len(positions))

    def shadow(self, offset=0, alpha=1):
        bind_texture(3, 0)  # avoid read/write feedback across consecutive frames
        gl("glBindFramebuffer", None, U, U)(FRAMEBUFFER, self.shadow_fbo)
        gl("glEnable", None, U)(0x0B71)
        gl("glDepthMask", None, c.c_ubyte)(1)
        gl("glClearDepth", None, c.c_double)(1)
        gl("glClear", None, U)(0x100)
        gl("glUseProgram", None, U)(self.programs[0])
        bone = identity()
        bone[12] = offset
        self.uniform(1, bone * 512)
        self.material(alpha)
        self.vertices([(-0.8,-0.1,0.25), (0.8,-0.1,0.25), (0,0.9,0.25)])
        depth = c.c_float()
        gl("glReadPixels", None, I, I, I, I, U, U, P)(32, 44, 1, 1, 0x1902, FLOAT, c.byref(depth))
        check(gl("glGetError", U)() == 0, "shadow draw GL error")
        return depth.value

    def receiver(self, wall=False):
        gl("glBindFramebuffer", None, U, U)(FRAMEBUFFER, self.color_fbo)
        gl("glDisable", None, U)(0x0B71)
        gl("glUseProgram", None, U)(self.programs[2 if wall else 1])
        bind_texture(3, self.depth)
        self.uniform(1, identity() * 512)
        self.material(1)
        self.vertices([(-1,-1,0.5), (1,-1,0.5), (1,1,0.5),
                       (-1,-1,0.5), (1,1,0.5), (-1,1,0.5)])
        values = []
        for y in (44, 14):
            pixel = (c.c_float * 4)()
            gl("glReadPixels", None, I, I, I, I, U, U, P)(32, y, 1, 1, 0x1908, FLOAT, pixel)
            values.append(list(pixel))
        check(gl("glGetError", U)() == 0, "receiver draw GL error")
        return values

    def close(self):
        # Release the attachment owners after the framebuffers.
        gl("glBindFramebuffer", None, U, U)(FRAMEBUFFER, 0)
        gl("glDeleteFramebuffers", None, I, c.POINTER(U))(2, (U*2)(self.shadow_fbo, self.color_fbo))
        gl("glDeleteTextures", None, I, c.POINTER(U))(3, (U*3)(self.depth, self.color, self.white))
        gl("glDeleteBuffers", None, I, c.POINTER(U))(5, (U*5)(*self.buffers))
        gl("glDeleteVertexArrays", None, I, c.POINTER(U))(1, c.byref(U(self.vao)))
        for value in self.programs:
            gl("glDeleteProgram", None, U)(value)


def run_tests(directory):
    clear_program = program(directory, "vs_clear_wall_gl.glsl", "ps_clear_wall_gl.glsl")
    gl("glDeleteProgram", None, U)(clear_program)
    runner = Runner(directory)
    try:
        check(abs(runner.shadow() - 0.25) < 0.0001, "GL clip depth conversion")
        wall_upper, wall_lower = runner.receiver(wall=True)
        check(abs(wall_upper[3] - 0.25) < 0.0001 and abs(wall_lower[3]) < 0.0001,
              f"wall shadow coverage/transparent background: {wall_upper}, {wall_lower}")
        check(max(abs(x) for x in wall_upper[:3]) < 0.0001, "wall shadow must be black")
        upper, lower = runner.receiver()
        check(upper[0] < 0.3 and lower[0] > 0.7, f"shadow/Y orientation: {upper}, {lower}")
        check(abs(upper[3] - 1) < 0.0001, "shadow must preserve receiver alpha")
        runner.uniform(2, identity() + [1.0, 0.0, 0.0, 0.0])
        check(runner.receiver()[0][0] > 0.7, "disabled self shadow must stay lit")
        check(abs(runner.receiver(wall=True)[0][3] - 0.25) < 0.0001, "wall shadow must remain enabled")
        check(abs(runner.shadow() - 0.25) < 0.0001, "disabled self shadow must still cast")
        runner.uniform(2, identity() + [0.0] * 4)
        check(abs(runner.shadow(alpha=0) - 1) < 0.0001, "transparent caster is discarded")
        check(runner.receiver()[0][0] > 0.7, "transparent caster must not shadow receiver")
        check(abs(runner.receiver(wall=True)[0][3]) < 0.0001, "wall must not retain old shadow")
        check(abs(runner.shadow(offset=2) - 1) < 0.0001, "skinned caster moves out of view")
        check(runner.receiver()[0][0] > 0.7, "moved caster must not leave a stale shadow")
        runner.shadow()
        check(runner.receiver()[0][0] < 0.3, "shadow restored on later frames")
        print("OpenGL shadow depth, alpha, skinning, receiver lighting and Y orientation passed")
    finally:
        runner.close()


if __name__ == "__main__":
    check(len(sys.argv) == 2, "usage: gl_shadow_tests.py <generated OpenGL shader directory>")
    run_with_context(lambda: run_tests(Path(sys.argv[1])))

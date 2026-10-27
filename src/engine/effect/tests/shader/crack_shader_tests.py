"""Check crack transparency, opacity and parallax using the existing hidden WGL harness."""
import ctypes as c
from pathlib import Path
import math
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "renderer/tests/skinning"))
from gl_skinning_tests import check, gl, run_with_context
from gl_shadow_tests import program, object_id, texture, framebuffer, bind_texture

U = c.c_uint
I = c.c_int
P = c.c_void_p
SIZE = 64
ARRAY_BUFFER = 0x8892
UNIFORM_BUFFER = 0x8A11
FLOAT = 0x1406
RGBA = 0x1908
RGBA32F = 0x8814
STATIC_DRAW = 0x88E4


def run(directory):
    pipeline = program(directory, "vs_crack_gl.glsl", "ps_crack_gl.glsl")
    block = gl("glGetUniformBlockIndex", U, U, c.c_char_p)(pipeline, b"Crack")
    check(block != 0xFFFFFFFF, "Crack uniform block must survive shader conversion")
    gl("glUniformBlockBinding", None, U, U, U)(pipeline, block, 0)
    vao = object_id("glGenVertexArrays")
    gl("glBindVertexArray", None, U)(vao)
    vertices = (c.c_float * 18)(-1, -1, 0, 1, -1, 0, 1, 1, 0, -1, -1, 0, 1, 1, 0, -1, 1, 0)
    vertex_buffer = object_id("glGenBuffers")
    gl("glBindBuffer", None, U, U)(ARRAY_BUFFER, vertex_buffer)
    gl("glBufferData", None, U, c.c_ssize_t, P, U)(ARRAY_BUFFER, c.sizeof(vertices), vertices, STATIC_DRAW)
    gl("glEnableVertexAttribArray", None, U)(0)
    gl("glVertexAttribPointer", None, U, I, U, c.c_ubyte, I, P)(0, 3, FLOAT, 0, 12, None)
    uniform_buffer = object_id("glGenBuffers")
    gl("glBindBuffer", None, U, U)(UNIFORM_BUFFER, uniform_buffer)
    gl("glBindBufferBase", None, U, U, U)(UNIFORM_BUFFER, 0, uniform_buffer)
    # Constant cavity depth isolates the expected POM displacement: du = slope * scale * depth.
    field = []
    for y in range(SIZE):
        for x in range(SIZE):
            coverage = float(SIZE // 3 < x < SIZE // 2)
            field.extend((0.5, coverage, 0, 0))
    mask = texture(RGBA32F, RGBA, SIZE, SIZE, (c.c_float * len(field))(*field))
    target = texture(RGBA32F, RGBA, SIZE, SIZE)
    framebuffer(target, 0x8CE0, True)
    bind_texture(0, mask)
    gl("glViewport", None, I, I, I, I)(0, 0, SIZE, SIZE)
    gl("glDisable", None, U)(0x0BE2)  # Inspect the shader's straight-alpha output before blending.

    def render(view, opacity):
        uniform = (c.c_float * 8)(view, 0, 0.1, opacity, 1 / SIZE, 1 / SIZE, 0, 0)
        gl("glBufferData", None, U, c.c_ssize_t, P, U)(UNIFORM_BUFFER, c.sizeof(uniform), uniform, STATIC_DRAW)
        gl("glDrawArrays", None, U, I, I)(4, 0, 6)
        pixels = (c.c_float * (SIZE * SIZE * 4))()
        gl("glReadPixels", None, I, I, I, I, U, U, P)(0, 0, SIZE, SIZE, RGBA, FLOAT, pixels)
        check(gl("glGetError", U)() == 0, "OpenGL draw/readback must succeed")
        check(all(math.isfinite(value) for value in pixels), "all output channels must be finite")
        return list(pixels)[3::4]

    normal = render(0, 1)
    check(any(alpha == 0 for alpha in normal), "intact regions stay transparent")
    check(any(alpha > 0.9 for alpha in normal), "cracks are visible")
    faded = render(0, 0.25)
    check(all(abs(a * 0.25 - b) < 0.00001 for a, b in zip(normal, faded)), "opacity scales alpha linearly")
    check(all(alpha == 0 for alpha in render(1, 0)), "zero opacity is fully transparent")
    shifted = render(1, 1)
    def centroid(alpha):
        return sum((index % SIZE) * value for index, value in enumerate(alpha)) / sum(alpha)
    expected_shift = SIZE * 1 * 0.1 * 0.5
    check(abs(centroid(shifted) - centroid(normal) - expected_shift) < 1,
          "POM must shift the crack by slope * depth scale * sampled depth")
    print("PASS: crack GPU transparency, fade, finite output and POM displacement")


if __name__ == "__main__":
    run_with_context(lambda: run(Path(sys.argv[1])))

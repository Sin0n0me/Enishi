"""Compile the shared HLSL source and emit Vulkan SPIR-V or OpenGL GLSL."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile


def normalize_block_names(path):
    # DXC prefixes block type names with "type."; the engine identifies blocks by HLSL name.
    words = list(struct.unpack("<%dI" % (path.stat().st_size // 4), path.read_bytes()))
    output = words[:5]
    offset = 5
    while offset < len(words):
        # SPIR-V instruction header = (word count << 16) | opcode.
        count = words[offset] >> 16
        opcode = words[offset] & 0xffff
        if count == 0 or offset + count > len(words):
            raise ValueError("Invalid SPIR-V instruction")
        instruction = words[offset:offset + count]
        if opcode == 5:  # OpName: target id, null-terminated UTF-8 name.
            name = struct.pack("<%dI" % (count - 2), *instruction[2:]).split(b"\0", 1)[0]
            if name.startswith(b"type."):
                name = name[len(b"type."):] + b"\0"
                name += b"\0" * ((-len(name)) % 4)
                encoded = list(struct.unpack("<%dI" % (len(name) // 4), name))
                instruction = [((len(encoded) + 2) << 16) | opcode, instruction[1], *encoded]
        output.extend(instruction)
        offset += count
    path.write_bytes(struct.pack("<%dI" % len(output), *output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend", choices=("opengl", "vulkan"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--dxc", required=True)
    parser.add_argument("--cross")
    parser.add_argument("--validator", required=True)
    args = parser.parse_args()
    vertex = args.source.name.startswith("vs_")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # Publish only a validated, fully converted artifact; failed builds leave no partial output.
    with tempfile.TemporaryDirectory(prefix="shader-", dir=args.output.parent) as temporary:
        spirv = Path(temporary) / "shader.spv"
        result = spirv
        command = [args.dxc, "-spirv", "-T", "vs_6_0" if vertex else "ps_6_0",
                   "-E", "main", "-fspv-target-env=vulkan1.1", "-D", "ENISHI_SPIRV=1",
                   "-D", "ENISHI_" + args.backend.upper() + "=1", "-Fo", str(spirv), str(args.source)]
        if args.backend == "vulkan" and vertex:
            command.append("-fvk-invert-y")
        subprocess.run(command, check=True)
        normalize_block_names(spirv)
        subprocess.run([args.validator, "--target-env", "vulkan1.1", str(spirv)], check=True)
        if args.backend == "opengl":
            if args.cross is None:
                parser.error("--cross is required for OpenGL")
            command = [args.cross, str(spirv), "--version", "400", "--no-es", "--no-420pack-extension", "--output", str(Path(temporary) / "shader.glsl")]
            if vertex:
                command.append("--fixup-clipspace")
            # GLSL 4.0 links varyings by name; DXC gives stage input/output different names.
            for location, name in enumerate(("uv", "view_position", "view_normal", "view_light", "shadow_position", "normal")):
                command += ["--rename-interface-variable", "out" if vertex else "in", str(location), name]
            subprocess.run(command, check=True)
            result = Path(temporary) / "shader.glsl"
        result.replace(args.output)


if __name__ == "__main__":
    main()

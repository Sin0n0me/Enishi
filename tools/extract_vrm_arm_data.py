"""Extract numeric arm-rig diagnostics from a local VRM without exporting model assets.

Only standard humanoid roles, anonymized hierarchy IDs, node transforms and selected
inverse bind matrices are exported. No names, paths, metadata, geometry or images
are included. Output matrices use glTF column-major order; coordinates are unchanged.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import struct
import sys


JSON_CHUNK = 0x4E4F534A
BIN_CHUNK = 0x004E4942
FLOAT = 5126
IDENTITY = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
ROLES = (
    "hips", "spine", "chest", "upperChest", "neck", "head", "leftEye", "rightEye",
    "leftShoulder", "leftUpperArm", "leftLowerArm", "leftHand",
    "rightShoulder", "rightUpperArm", "rightLowerArm", "rightHand",
)


def index(value: object, count: int) -> int:
    if type(value) is not int or not 0 <= value < count:
        raise ValueError("Invalid array index in VRM")
    return value


def integer(value: object) -> int:
    if type(value) is not int or value < 0:
        raise ValueError("Invalid count or offset in VRM")
    return value


def numbers(value: object, count: int) -> list:
    if not isinstance(value, list) or len(value) != count:
        raise ValueError("Invalid transform dimensions")
    if any(type(x) not in (int, float) or not math.isfinite(x) for x in value):
        raise ValueError("Transform contains non-finite or non-numeric values")
    return value


def read_glb(path: Path) -> tuple[dict, bytes]:
    data = path.read_bytes()
    if len(data) < 12:
        raise ValueError("Truncated GLB header")
    magic, version, length = struct.unpack_from("<4sII", data)
    if magic != b"glTF" or version != 2 or length != len(data):
        raise ValueError("Expected a complete GLB version 2 VRM file")
    chunks = {}
    offset = 12
    first = True
    while offset < length:
        if offset + 8 > length:
            raise ValueError("Truncated GLB chunk header")
        size, kind = struct.unpack_from("<II", data, offset)
        offset += 8
        if size % 4 or offset + size > length or (first and kind != JSON_CHUNK):
            raise ValueError("Invalid GLB chunk")
        first = False
        if kind in (JSON_CHUNK, BIN_CHUNK):
            if kind in chunks:
                raise ValueError("Duplicate GLB chunk")
            chunks[kind] = data[offset:offset + size]
        offset += size
    document = json.loads(chunks[JSON_CHUNK].decode("utf-8"))
    return document, chunks.get(BIN_CHUNK, b"")


def read_elements(document: dict, binary: bytes, view_id: int, offset: int,
                  count: int, fmt: str, use_stride: bool = True) -> list[tuple]:
    views = document.get("bufferViews", [])
    view = views[index(view_id, len(views))]
    buffers = document.get("buffers", [])
    buffer_id = index(view.get("buffer", 0), len(buffers))
    buffer = buffers[buffer_id]
    if buffer_id != 0 or "uri" in buffer:
        raise ValueError("External buffers are not read by this tool")
    start = integer(view.get("byteOffset", 0))
    size = integer(view["byteLength"])
    element_size = struct.calcsize(fmt)
    stride = integer(view.get("byteStride", element_size)) if use_stride else element_size
    offset = integer(offset)
    count = integer(count)
    end = offset + (count - 1) * stride + element_size if count else offset
    if stride < element_size or end > size or start + size > min(
            len(binary), integer(buffer["byteLength"])):
        raise ValueError("Accessor exceeds its buffer view")
    return [struct.unpack_from(fmt, binary, start + offset + i * stride) for i in range(count)]


def inverse_bind_matrices(document: dict, binary: bytes, skin: dict) -> list[list]:
    count = len(skin["joints"])
    if "inverseBindMatrices" not in skin:
        return [IDENTITY.copy() for _ in range(count)]
    accessors = document.get("accessors", [])
    accessor = accessors[index(skin["inverseBindMatrices"], len(accessors))]
    if (accessor.get("type") != "MAT4" or accessor.get("componentType") != FLOAT
            or accessor.get("count") != count or accessor.get("normalized", False)):
        raise ValueError("Expected FLOAT MAT4 inverse bind matrices matching skin joints")
    matrices = [[0.0] * 16 for _ in range(count)]
    if "bufferView" in accessor:
        matrices = [list(row) for row in read_elements(document, binary, accessor["bufferView"],
                    accessor.get("byteOffset", 0), count, "<16f")]
    if "sparse" in accessor:
        sparse = accessor["sparse"]
        sparse_count = integer(sparse["count"])
        if sparse_count > count:
            raise ValueError("Sparse accessor count exceeds matrix count")
        indices = sparse["indices"]
        formats = {5121: "<B", 5123: "<H", 5125: "<I"}
        if indices["componentType"] not in formats:
            raise ValueError("Invalid sparse index type")
        slots = read_elements(document, binary, indices["bufferView"],
                              indices.get("byteOffset", 0), sparse_count,
                              formats[indices["componentType"]], False)
        values = sparse["values"]
        rows = read_elements(document, binary, values["bufferView"],
                             values.get("byteOffset", 0), sparse_count, "<16f", False)
        previous = -1
        for (slot,), row in zip(slots, rows):
            index(slot, count)
            if slot <= previous:
                raise ValueError("Sparse indices must be strictly increasing")
            matrices[slot] = list(row)
            previous = slot
    return [numbers(matrix, 16) for matrix in matrices]


def extract(document: dict, binary: bytes) -> dict:
    nodes = document.get("nodes", [])
    extensions = document.get("extensions", {})
    if "VRMC_vrm" in extensions:
        version = "1"
        humanoid = extensions["VRMC_vrm"]["humanoid"]["humanBones"]
    elif "VRM" in extensions:
        version = "0"
        humanoid = {bone["bone"]: bone for bone in extensions["VRM"]["humanoid"]["humanBones"]}
    else:
        raise ValueError("VRM humanoid extension not found")
    roles = {role: index(humanoid[role]["node"], len(nodes)) for role in ROLES if role in humanoid}
    if not any(role.endswith("UpperArm") for role in roles):
        raise ValueError("No humanoid upper-arm bones found")
    parents = {}
    for parent, node in enumerate(nodes):
        for child in node.get("children", []):
            index(child, len(nodes))
            if child in parents:
                raise ValueError("Node has multiple parents")
            parents[child] = parent
    selected = set()
    for bone in roles.values():
        chain = set()
        while bone is not None:
            if bone in chain:
                raise ValueError("Cycle in arm hierarchy")
            chain.add(bone)
            selected.add(bone)
            bone = parents.get(bone)
    ids = {bone: f"n{i}" for i, bone in enumerate(sorted(selected))}
    output_nodes = []
    for bone in sorted(selected):
        node = nodes[bone]
        item = {"id": ids[bone], "parent": ids.get(parents.get(bone)),
                "roles": [role for role, value in roles.items() if value == bone]}
        if "matrix" in node:
            item["matrix"] = numbers(node["matrix"], 16)
        else:
            item.update(translation=numbers(node.get("translation", [0, 0, 0]), 3),
                        rotation_xyzw=numbers(node.get("rotation", [0, 0, 0, 1]), 4),
                        scale=numbers(node.get("scale", [1, 1, 1]), 3))
        output_nodes.append(item)
    skins = []
    source_skins = document.get("skins", [])
    used_skins = {index(node["skin"], len(source_skins)) for node in nodes if "skin" in node}
    for skin_id in sorted(used_skins):
        skin = source_skins[skin_id]
        joints = [index(joint, len(nodes)) for joint in skin["joints"]]
        if not selected.intersection(joints):
            continue
        matrices = inverse_bind_matrices(document, binary, skin)
        skins.append({"id": f"s{len(skins)}", "joints": [
            {"node": ids[joint], "inverse_bind_matrix": matrices[i]}
            for i, joint in enumerate(joints) if joint in selected]})
    return {"schema_version": 1, "vrm_version": version,
            "coordinate_space": "source glTF; meters; matrices column-major; quaternion xyzw",
            "nodes": output_nodes, "skins": skins}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model", type=Path, help="Local .vrm file (never uploaded)")
    parser.add_argument("--output", "-o", type=Path,
                        help="New JSON file; existing files are never overwritten (default: stdout)")
    args = parser.parse_args()
    try:
        result = extract(*read_glb(args.model))
        text = json.dumps(result, indent=2, allow_nan=False) + "\n"
        if args.output is None:
            sys.stdout.write(text)
        else:
            with args.output.open("x", encoding="utf-8", newline="\n") as stream:
                stream.write(text)
        return 0
    except (OSError, ValueError, KeyError, TypeError, IndexError, AttributeError,
            struct.error, OverflowError, RecursionError):
        # Do not echo source-controlled strings, metadata or private filesystem paths.
        print("Extraction failed: check the VRM structure, embedded buffers, file permissions "
              "and that the output file does not already exist.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

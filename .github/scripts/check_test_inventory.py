"""Fail early when a CI test suite, executable, or rendering asset is missing."""

import argparse
import json
from pathlib import Path
import subprocess
import sys

COMMON = {
    "foundation_header_tests", "foundation_option_tests", "foundation_result_tests", "foundation_runtime_tests",
    "pmd_morph_tests", "pmx_tests", "vrm_tests", "vrm_json_tests", "vertex_morph_tests",
    "material_tests", "animation_runtime_tests", "bone_layer_tests", "physics_runtime_tests",
    "ik_tests", "ColliderTests",
}
CORE = {"enishi_script_tests", "enishi_cursor_tests", "enishi_animation_pose_tests",
        "enishi_ecs_tests", "renderer_resource_tests"}
OPENGL = {"opengl_shadow_tests", "opengl_shadow_shaders", "opengl_skinning_shaders",
          "opengl_crack_shaders", "opengl_test_context"}
SHADERS = ("vs_model", "ps_model", "vs_shadow_map", "ps_shadow_map",
           "vs_clear_wall", "ps_clear_wall", "vs_crack", "ps_crack")


def required_tests(scope, backend, windows):
    names = COMMON | ({"skinning_tests", "shadow_tests"} if windows else set())
    if scope != "common":
        names |= CORE
    if scope == "desktop":
        names |= {"crack_render_tests"}
        names |= OPENGL if backend == "opengl" else {"vulkan_render_tests"}
    return names


def validate(inventory, build, scope, backend, windows):
    tests = inventory.get("tests", [])
    names = [test["name"] for test in tests]
    problems = []
    if len(set(names)) != len(names):
        problems.append("Duplicate CTest names")
    missing = required_tests(scope, backend, windows) - set(names)
    if missing:
        problems.append("Missing CTest registrations: " + ", ".join(sorted(missing)))
    for test in tests:
        name = test["name"]
        command = test.get("command", [])
        properties = {item["name"]: item["value"] for item in test.get("properties", [])}
        if not command or not Path(command[0]).is_file():
            problems.append(f"{name}: test executable/interpreter was not built or found")
        if properties.get("DISABLED", False):
            problems.append(f"{name}: registered test is disabled")
        if name in {"crack_render_tests", "vulkan_render_tests", "opengl_shadow_tests"}:
            working = Path(properties.get("WORKING_DIRECTORY", ""))
            if working.resolve() != build.resolve():
                problems.append(f"{name}: working directory must be the shader deployment root")
    if scope == "desktop":
        suffix = "_gl.glsl" if backend == "opengl" else "_vk.spv"
        for name in SHADERS:
            asset = build / "shader" / backend / (name + suffix)
            if not asset.is_file() or asset.stat().st_size == 0:
                problems.append(f"Missing or empty shader asset: {asset}")
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--config", required=True)
    parser.add_argument("--scope", choices=("common", "core", "desktop"), required=True)
    parser.add_argument("--backend", choices=("opengl", "vulkan"))
    args = parser.parse_args()
    if args.scope == "desktop" and args.backend is None:
        parser.error("--backend is required for desktop tests")
    directory = args.build / "engine/tests" if args.scope == "common" else args.build
    result = subprocess.run(["ctest", "--test-dir", str(directory), "-C", args.config,
                             "--show-only=json-v1"], check=True, capture_output=True, text=True)
    inventory = json.loads(result.stdout)
    problems = validate(inventory, args.build, args.scope, args.backend, sys.platform == "win32")
    if problems:
        raise SystemExit("CI test preflight failed:\n" + "\n".join(problems))
    print(f"CI test preflight: {len(inventory['tests'])} registered tests checked")


if __name__ == "__main__":
    main()

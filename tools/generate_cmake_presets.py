"""Generate shared CMake presets; edit the axes and exclusions here, not the JSON."""

from __future__ import annotations

import argparse
from itertools import product
import json
from pathlib import Path


OUTPUT = Path(__file__).resolve().parents[1] / "src" / "CMakePresets.json"
OPERATING_SYSTEMS = {"windows": "Windows", "linux": "Linux", "macos": "Darwin"}
COMPILERS = {
    "visualstudio": {"generator": "Visual Studio 17 2022", "architecture": "x64"},
    "visualstudio2026": {"generator": "Visual Studio 18 2026", "architecture": "x64"},
    "clang": {"generator": "Ninja", "cacheVariables": {
        "CMAKE_C_COMPILER": "clang", "CMAKE_CXX_COMPILER": "clang++"}},
    "gcc": {"generator": "Ninja", "cacheVariables": {
        "CMAKE_C_COMPILER": "gcc", "CMAKE_CXX_COMPILER": "g++"}},
}
BACKENDS = {
    "directx": {"USE_OPENGL40": False, "USE_VULKAN": False},
    "opengl": {"USE_OPENGL40": True, "USE_VULKAN": False},
    "vulkan": {"USE_OPENGL40": False, "USE_VULKAN": True},
}
CONFIGURATIONS = ("Debug", "Release")
# Each rule excludes combinations matching all its fields; omitted axes are unrestricted.
EXCLUSIONS = (
    ({"os": ("linux", "macos"), "backend": ("directx",)}, "DirectX requires Windows"),
    ({"os": ("linux", "macos"), "compiler": ("visualstudio", "visualstudio2026")},
     "Visual Studio generators require Windows"),
    ({"os": ("macos",), "backend": ("vulkan",)},
     "The macOS desktop implementation uses OpenGL; MoltenVK is not a project dependency"),
    ({"os": ("macos",), "compiler": ("gcc",)},
     "The Cocoa implementation requires the supported Clang Objective-C++ toolchain"),
)
MSBUILD_OPTIONS = ["/p:UseMultiToolTask=true", "/p:EnforceProcessCountAcrossBuilds=true"]


def host_condition(host: str) -> dict:
    return {"type": "equals", "lhs": "${hostSystemName}", "rhs": host}


def hidden_presets() -> list[dict]:
    presets = [
        {"name": "base", "hidden": True,
         "binaryDir": "${sourceDir}/../build/presets/${presetName}",
         "cacheVariables": {"CMAKE_EXPORT_COMPILE_COMMANDS": True}},
        {"name": "windows", "hidden": True, "inherits": "base",
         "condition": host_condition(OPERATING_SYSTEMS["windows"]),
         "toolchainFile": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
         "cacheVariables": {"VCPKG_TARGET_TRIPLET": "x64-windows"}},
        {"name": "unix", "hidden": True, "inherits": "base", "generator": "Ninja"},
    ]
    presets.extend({"name": name, "hidden": True, **settings}
                   for name, settings in COMPILERS.items())
    presets.extend({"name": name, "hidden": True, "cacheVariables": settings}
                   for name, settings in BACKENDS.items())
    return presets


def patterns() -> list[dict]:
    axes = {"os": OPERATING_SYSTEMS, "compiler": COMPILERS,
            "backend": BACKENDS, "config": CONFIGURATIONS}
    for rule, reason in EXCLUSIONS:
        for axis, values in rule.items():
            if axis not in axes or not values or not set(values).issubset(axes[axis]):
                raise ValueError(f"Invalid exclusion ({reason}): {axis}={values}")
    result = []
    for values in product(*axes.values()):
        pattern = dict(zip(axes, values))
        if any(all(pattern[axis] in values for axis, values in rule.items())
               for rule, _ in EXCLUSIONS):
            continue
        result.append(pattern)
    return result


def generate() -> dict:
    configure = hidden_presets()
    build = []
    for pattern in patterns():
        target_os = pattern["os"]
        compiler = pattern["compiler"]
        backend = pattern["backend"]
        config = pattern["config"]
        name = f"{target_os}-{backend}-{compiler}-{config.lower()}"
        variables = {"CMAKE_BUILD_TYPE": config}
        if target_os == "windows" and compiler == "gcc":
            variables["VCPKG_TARGET_TRIPLET"] = "x64-mingw-dynamic"
        if target_os == "macos":
            variables["CMAKE_OBJCXX_COMPILER"] = "clang++"
            variables["CMAKE_OSX_DEPLOYMENT_TARGET"] = "14.0"
        configure.append({
            "name": name, "displayName": f"{target_os} / {backend} / {compiler} / {config}",
            "inherits": [compiler, backend, "windows" if target_os == "windows" else "unix"],
            "condition": host_condition(OPERATING_SYSTEMS[target_os]),
            "cacheVariables": variables,
            "vendor": {"enishi/build/1.0": {
                "os": target_os, "backend": backend, "compiler": compiler, "config": config}},
        })
        build_preset = {"name": name, "configurePreset": name,
                        "configuration": config, "targets": ["enishi_engine"]}
        if COMPILERS[compiler]["generator"].startswith("Visual Studio "):
            build_preset["nativeToolOptions"] = MSBUILD_OPTIONS
        build.append(build_preset)
    return {
        "version": 5,
        "cmakeMinimumRequired": {"major": 3, "minor": 24, "patch": 0},
        "vendor": {"enishi/generator/1.0": {
            "source": "tools/generate_cmake_presets.py",
            "command": "python tools/generate_cmake_presets.py",
            "notice": "Generated file. Edit the generator, then regenerate."}},
        "configurePresets": configure,
        "buildPresets": build,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true",
                        help="Report stale or missing presets without writing files")
    args = parser.parse_args()
    contents = json.dumps(generate(), indent=2) + "\n"
    if args.check:
        if not OUTPUT.is_file() or OUTPUT.read_text(encoding="utf-8") != contents:
            print("CMake presets are stale. Run: python tools/generate_cmake_presets.py")
            return 1
        print("CMake presets are up to date.")
    else:
        OUTPUT.write_bytes(contents.encode("utf-8"))
        print(f"Generated {OUTPUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

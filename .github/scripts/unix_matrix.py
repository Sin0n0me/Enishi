"""Select Unix CI jobs from the supported CMake preset combinations."""

import argparse
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from generate_cmake_presets import patterns


RUNNERS = {
    "linux": (("ubuntu-24.04", "x64"),),
    "macos": (("macos-15", "arm64"), ("macos-15-intel", "x64")),
}
LINUX_COMPILERS = {"clang": ("clang-19", "clang++-19"), "gcc": ("gcc-14", "g++-14")}


def matrix(standalone=False):
    entries = []
    for pattern in patterns():
        target_os, backend, compiler, config = (pattern[key] for key in
                                               ("os", "backend", "compiler", "config"))
        if target_os not in RUNNERS:
            continue
        preset = f"{target_os}-{backend}-{compiler}-{config.lower()}"
        cc, cxx = LINUX_COMPILERS[compiler] if target_os == "linux" else ("clang", "clang++")
        for runner, arch in RUNNERS[target_os]:
            entries.append({**pattern, "preset": preset, "runner": runner, "arch": arch,
                            "cc": cc, "cxx": cxx, "artifact": f"enishi-{preset}-{arch}"})
    if standalone:
        # Backend-independent suites run once per OS/compiler/configuration/architecture.
        unique = {}
        for entry in entries:
            key = (entry["os"], entry["compiler"], entry["config"], entry["arch"])
            unique.setdefault(key, {name: value for name, value in entry.items()
                                    if name not in ("preset", "backend", "artifact")})
        entries = list(unique.values())
    return {"include": entries}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--standalone", action="store_true")
    print(json.dumps(matrix(parser.parse_args().standalone), separators=(",", ":")))

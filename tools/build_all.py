"""Build every native backend/compiler/configuration combination for the selected OS."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[1]
BACKENDS = {"windows": ("directx", "opengl", "vulkan"),
            "linux": ("opengl", "vulkan"), "macos": ("opengl", "vulkan")}
COMPILERS = {"windows": ("visualstudio", "clang", "gcc"),
             "linux": ("clang", "gcc"), "macos": ("clang", "gcc")}
CONFIGURATIONS = ("Debug", "Release")
HOST_OS = {"Windows": "windows", "Linux": "linux", "Darwin": "macos"}


@dataclass(frozen=True)
class Pattern:
    target_os: str
    backend: str
    compiler: str
    config: str

    def directory(self, root: Path) -> Path:
        return root / self.target_os / self.backend / self.compiler / self.config

    def label(self) -> str:
        return f"{self.target_os}/{self.backend}/{self.compiler}/{self.config}"


def visual_studio_generator() -> str:
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / (
        "Microsoft Visual Studio/Installer/vswhere.exe")
    if vswhere.is_file() and platform.system() == "Windows":
        result = subprocess.run(
            [str(vswhere), "-latest", "-products", "*", "-requires",
             "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationVersion"],
            capture_output=True, text=True, check=False)
        if result.returncode == 0 and result.stdout.strip().startswith("18."):
            return "Visual Studio 18 2026"
    return "Visual Studio 17 2022"


def package_options(pattern: Pattern) -> list[str]:
    toolchain = os.environ.get("CMAKE_TOOLCHAIN_FILE")
    if not toolchain:
        root = os.environ.get("VCPKG_ROOT") or os.environ.get("VCPKG_INSTALLATION_ROOT")
        executable = shutil.which("vcpkg")
        if not root and executable:
            root = str(Path(executable).resolve().parent)
        if root:
            toolchain = str(Path(root) / "scripts/buildsystems/vcpkg.cmake")
    if not toolchain:
        return []
    architecture = "arm64" if platform.machine().lower() in ("arm64", "aarch64") else "x64"
    if pattern.target_os == "windows":
        triplet = "x64-mingw-dynamic" if pattern.compiler == "gcc" else "x64-windows"
    else:
        suffix = "osx" if pattern.target_os == "macos" else "linux"
        triplet = f"{architecture}-{suffix}"
    options = [f"-DCMAKE_TOOLCHAIN_FILE={Path(toolchain).resolve()}",
               f"-DVCPKG_TARGET_TRIPLET={triplet}"]
    if os.environ.get("VCPKG_INSTALLED_DIR"):
        options.append(f"-DVCPKG_INSTALLED_DIR={Path(os.environ['VCPKG_INSTALLED_DIR']).resolve()}")
    return options


def commands(pattern: Pattern, root: Path, jobs: int, downloads: bool,
             vs_generator: str) -> tuple[list[str], list[str]]:
    directory = pattern.directory(root)
    build = directory / "build"
    artifacts = directory / "artifacts"
    configure = ["cmake", "-S", str(PROJECT_ROOT / "src"), "-B", str(build),
                 f"-DUSE_OPENGL40={'ON' if pattern.backend == 'opengl' else 'OFF'}",
                 f"-DUSE_VULKAN={'ON' if pattern.backend == 'vulkan' else 'OFF'}",
                 f"-DCMAKE_BUILD_TYPE={pattern.config}",
                 f"-DPython_EXECUTABLE={sys.executable}",
                 f"-DPython3_EXECUTABLE={sys.executable}"]
    configure += package_options(pattern)
    configure += [f"-DFETCHCONTENT_FULLY_DISCONNECTED={'OFF' if downloads else 'ON'}",
                  f"-DFETCHCONTENT_UPDATES_DISCONNECTED={'OFF' if downloads else 'ON'}",
                  f"-DVCPKG_MANIFEST_INSTALL={'ON' if downloads else 'OFF'}"]
    # Per-config overrides prevent multi-config generators from adding a second Debug/Release.
    for kind in ("RUNTIME", "LIBRARY", "ARCHIVE", "PDB"):
        for suffix in ("", "_" + pattern.config.upper()):
            configure.append(f"-DCMAKE_{kind}_OUTPUT_DIRECTORY{suffix}={artifacts}")
    if pattern.compiler == "visualstudio":
        configure += ["-G", vs_generator, "-A", "x64"]
    else:
        compiler, cxx = (("clang", "clang++") if pattern.compiler == "clang" else ("gcc", "g++"))
        configure += ["-G", "Ninja", f"-DCMAKE_C_COMPILER={compiler}", f"-DCMAKE_CXX_COMPILER={cxx}"]
    build_command = ["cmake", "--build", str(build), "--config", pattern.config,
                     "--parallel", str(jobs)]
    if pattern.compiler == "visualstudio":
        build_command += ["--", "/p:UseMultiToolTask=true", "/p:EnforceProcessCountAcrossBuilds=true",
                          f"/p:MultiProcMaxCount={jobs}"]
    return configure, build_command


def run_pattern(pattern: Pattern, root: Path, command_pair: tuple[list[str], list[str]],
                environment: dict[str, str]) -> dict[str, object]:
    directory = pattern.directory(root)
    result: dict[str, object] = {"pattern": pattern.label(), "status": "failed",
                               "artifacts": str(directory / "artifacts")}
    try:
        directory.mkdir(parents=True, exist_ok=True)
        with (directory / "build.log").open("w", encoding="utf-8") as log:
            for stage, command in zip(("configure", "build"), command_pair):
                result["stage"] = stage
                log.write(shlex.join(command) + "\n")
                log.flush()
                completed = subprocess.run(command, cwd=PROJECT_ROOT, env=environment,
                                           stdout=log, stderr=subprocess.STDOUT, check=False)
                result["returncode"] = completed.returncode
                if completed.returncode != 0:
                    return result
        result["stage"] = "collect"
        executable = "enishi_engine.exe" if pattern.target_os == "windows" else "enishi_engine"
        if not (directory / "artifacts" / executable).is_file():
            raise FileNotFoundError(f"Expected output was not produced: {executable}")
        shaders = directory / "build" / "shader"
        if shaders.is_dir():
            shutil.copytree(shaders, directory / "artifacts" / "shader", dirs_exist_ok=True)
        result["status"] = "success"
    except OSError as error:
        result["returncode"] = 1
        result["error"] = str(error)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, epilog=(
        "Requires installed compilers, Ninja, SDKs and project dependencies. "
        "Run Windows builds from a Visual Studio developer shell. No cross-compilation. "
        "Dependency fetching follows the existing build scripts; use --offline to disable it."))
    parser.add_argument("os", choices=tuple(BACKENDS))
    parser.add_argument("--output", type=Path, default=PROJECT_ROOT / "build" / "all")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    parser.add_argument("--dry-run", action="store_true", help="Print all commands without building or writing files")
    parser.add_argument("--offline", action="store_true", help="Disable CMake/vcpkg/Cargo dependency fetching")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if not args.dry_run and HOST_OS.get(platform.system()) != args.os:
        parser.error("The selected OS must match the host OS; use --dry-run to inspect another OS")
    root = args.output.resolve()
    generator = visual_studio_generator() if args.os == "windows" else ""
    environment = os.environ.copy()
    if args.offline:
        environment["CARGO_NET_OFFLINE"] = "true"
        environment["RUSTUP_AUTO_INSTALL"] = "0"
    results = []
    for backend in BACKENDS[args.os]:
        for compiler in COMPILERS[args.os]:
            for config in CONFIGURATIONS:
                pattern = Pattern(args.os, backend, compiler, config)
                pair = commands(pattern, root, args.jobs, not args.offline, generator)
                print(f"[{pattern.label()}]", flush=True)
                if args.dry_run:
                    for command in pair:
                        print(shlex.join(command))
                    continue
                result = run_pattern(pattern, root, pair, environment)
                results.append(result)
                print(f"  {result['status']}: {pattern.directory(root)}", flush=True)
    if args.dry_run:
        return 0
    summary = root / args.os / "summary.json"
    summary.write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    failed = sum(result["status"] != "success" for result in results)
    print(f"Succeeded: {len(results) - failed}; failed: {failed}. Summary: {summary}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

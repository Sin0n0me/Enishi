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


def build_command(pattern: Pattern, script: Path) -> list[str]:
    arguments = [str(script), pattern.backend, pattern.config, pattern.compiler]
    if pattern.target_os == "windows":
        return ["cmd", "/d", "/c", "call", *arguments]
    return ["bash", *arguments]


def run_pattern(pattern: Pattern, root: Path, command: list[str],
                environment: dict[str, str]) -> dict[str, object]:
    directory = pattern.directory(root)
    result: dict[str, object] = {"pattern": pattern.label(), "status": "failed",
                               "artifacts": str(directory / "artifacts")}
    try:
        directory.mkdir(parents=True, exist_ok=True)
        with (directory / "build.log").open("w", encoding="utf-8") as log:
            result["stage"] = "build"
            heading = f"[{pattern.label()} / build] {shlex.join(command)}"
            print(heading, flush=True)
            log.write(heading + "\n")
            log.flush()
            with subprocess.Popen(command, cwd=PROJECT_ROOT, env=environment,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  text=True, errors="replace", bufsize=1) as process:
                for line in process.stdout:
                    print(line, end="", flush=True)
                    log.write(line)
                    log.flush()
                result["returncode"] = process.wait()
            if result["returncode"] != 0:
                return result
        result["stage"] = "collect"
        executable = "enishi_engine.exe" if pattern.target_os == "windows" else "enishi_engine"
        executable_path = directory / "artifacts" / executable
        if not executable_path.is_file():
            raise FileNotFoundError(f"Expected output was not produced: {executable}")
        shaders = directory / "build" / "shader"
        if shaders.is_dir():
            shutil.copytree(shaders, directory / "artifacts" / "shader", dirs_exist_ok=True)
        result["status"] = "success"
        result["executable"] = str(executable_path)
        print(f"  Executable: {executable_path}", flush=True)
    except OSError as error:
        result["returncode"] = 1
        result["error"] = str(error)
        print(f"  Error: {error}", file=sys.stderr, flush=True)
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
    script_format = "cmd" if args.os == "windows" else "sh"
    script = PROJECT_ROOT / "build" / f"build.{script_format}"
    generate = [sys.executable, str(PROJECT_ROOT / "build" / "generate_build.py"),
                "--format", script_format]
    print(f"Generate: {shlex.join(generate)}", flush=True)
    if not args.dry_run:
        generated = subprocess.run(generate, cwd=PROJECT_ROOT, check=False)
        if generated.returncode != 0:
            return generated.returncode
    environment = os.environ.copy()
    environment["CMAKE_BUILD_PARALLEL_LEVEL"] = str(args.jobs)
    environment["PYTHON_EXECUTABLE"] = sys.executable
    environment["ENISHI_OFFLINE"] = "1" if args.offline else "0"
    if args.offline:
        environment["CARGO_NET_OFFLINE"] = "true"
        environment["RUSTUP_AUTO_INSTALL"] = "0"
    results = []
    for backend in BACKENDS[args.os]:
        for compiler in COMPILERS[args.os]:
            for config in CONFIGURATIONS:
                pattern = Pattern(args.os, backend, compiler, config)
                command = build_command(pattern, script)
                pattern_environment = environment.copy()
                pattern_environment["ENISHI_BUILD_DIR"] = str(pattern.directory(root) / "build")
                pattern_environment["ENISHI_ARTIFACT_DIR"] = str(pattern.directory(root) / "artifacts")
                if args.os == "windows":
                    pattern_environment["VCPKG_TARGET_TRIPLET"] = (
                        "x64-mingw-dynamic" if compiler == "gcc" else "x64-windows")
                print(f"[{pattern.label()}]", flush=True)
                if args.dry_run:
                    print(f"  Build: {pattern_environment['ENISHI_BUILD_DIR']}")
                    print(f"  Artifacts: {pattern_environment['ENISHI_ARTIFACT_DIR']}")
                    print(shlex.join(command))
                    continue
                result = run_pattern(pattern, root, command, pattern_environment)
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

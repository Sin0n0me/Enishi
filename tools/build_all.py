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
HOST_OS = {"Windows": "windows", "Linux": "linux", "Darwin": "macos"}
PRESETS = PROJECT_ROOT / "src" / "CMakePresets.json"


@dataclass(frozen=True)
class Pattern:
    preset: str
    target_os: str
    backend: str
    compiler: str
    config: str

    def directory(self, root: Path) -> Path:
        return root / self.target_os / self.backend / self.compiler / self.config

    def label(self) -> str:
        return f"{self.target_os}/{self.backend}/{self.compiler}/{self.config}"


def build_commands(pattern: Pattern, root: Path, offline: bool) -> list[list[str]]:
    directory = pattern.directory(root)
    artifacts = directory / "artifacts"
    configure = ["cmake", "--preset", pattern.preset, "-B", str(directory / "build")]
    for kind in ("RUNTIME", "LIBRARY", "PDB"):
        for suffix in ("", "_DEBUG", "_RELEASE"):
            configure.append(f"-DCMAKE_{kind}_OUTPUT_DIRECTORY{suffix}={artifacts}")
    if offline:
        configure += ["-DFETCHCONTENT_FULLY_DISCONNECTED=ON",
                      "-DFETCHCONTENT_UPDATES_DISCONNECTED=ON",
                      "-DVCPKG_MANIFEST_INSTALL=OFF", "-DENISHI_CARGO_OFFLINE=ON"]
    build_presets = json.loads(PRESETS.read_text(encoding="utf-8"))["buildPresets"]
    settings = next(item for item in build_presets if item["name"] == pattern.preset)
    build = ["cmake", "--build", str(directory / "build"), "--config", settings["configuration"],
             "--target", *settings["targets"]]
    if settings.get("nativeToolOptions"):
        build += ["--", *settings["nativeToolOptions"]]
    return [configure, build]


def run_pattern(pattern: Pattern, root: Path, commands: list[list[str]],
                environment: dict[str, str]) -> dict[str, object]:
    directory = pattern.directory(root)
    result: dict[str, object] = {"pattern": pattern.label(), "status": "failed",
                               "artifacts": str(directory / "artifacts")}
    try:
        directory.mkdir(parents=True, exist_ok=True)
        with (directory / "build.log").open("w", encoding="utf-8") as log:
            for stage, command in zip(("configure", "build"), commands):
                result["stage"] = stage
                heading = f"[{pattern.label()} / {stage}] {shlex.join(command)}"
                print(heading, flush=True)
                log.write(heading + "\n")
                log.flush()
                with subprocess.Popen(command, cwd=PROJECT_ROOT / "src", env=environment,
                                      stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                      text=True, errors="replace", bufsize=1) as process:
                    for line in process.stdout:
                        print(line, end="", flush=True)
                        log.write(line)
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
        "Dependency fetching follows CMake; use --offline to disable it."))
    parser.add_argument("os", choices=tuple(HOST_OS.values()))
    parser.add_argument("--output", type=Path, default=PROJECT_ROOT / "build" / "all")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    parser.add_argument("--dry-run", action="store_true", help="Print all commands without building or writing files")
    parser.add_argument("--offline", action="store_true", help="Disable CMake/vcpkg/Cargo dependency fetching")
    parser.add_argument("--compiler", action="append", help="Select compiler(s), e.g. visualstudio2026")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if not args.dry_run and HOST_OS.get(platform.system()) != args.os:
        parser.error("The selected OS must match the host OS; use --dry-run to inspect another OS")
    root = args.output.resolve()
    definitions = json.loads(PRESETS.read_text(encoding="utf-8"))["configurePresets"]
    patterns = []
    for preset in definitions:
        metadata = preset.get("vendor", {}).get("enishi/build/1.0")
        if metadata and metadata["os"] == args.os:
            patterns.append(Pattern(preset["name"], metadata["os"], metadata["backend"],
                                    metadata["compiler"], metadata["config"]))
    available = {pattern.compiler for pattern in patterns}
    if args.compiler and not set(args.compiler).issubset(available):
        parser.error(f"Available compilers: {', '.join(sorted(available))}")
    if args.compiler:
        patterns = [pattern for pattern in patterns if pattern.compiler in args.compiler]
    environment = os.environ.copy()
    environment["CMAKE_BUILD_PARALLEL_LEVEL"] = str(args.jobs)
    environment["MultiProcMaxCount"] = str(args.jobs)
    if args.offline:
        environment["CARGO_NET_OFFLINE"] = "true"
        environment["RUSTUP_AUTO_INSTALL"] = "0"
    results = []
    for pattern in patterns:
        commands = build_commands(pattern, root, args.offline)
        print(f"[{pattern.label()}]", flush=True)
        if args.dry_run:
            for command in commands:
                print(shlex.join(command))
            continue
        result = run_pattern(pattern, root, commands, environment)
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

"""Configure, build and run the existing regression suites without downloading dependencies."""

from __future__ import annotations

import argparse
from collections import deque
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[1]
STANDALONE = {
    "effect": "effect/tests/runtime",
    "pmd": "assets_system/tests/pmd",
    "pmx": "assets_system/tests/pmx",
    "vrm": "assets_system/tests/vrm",
    "morphs": "core/tests/morphs",
    "materials": "renderer/tests/materials",
    "animation": "animation/tests/runtime",
    "bone_layers": "core/tests/bone_layers",
    "physics": "core/tests/physics",
    "ik": "skinning_system/tests/ik",
    "skinning": "renderer/tests/skinning",
}
INTEGRATED = {
    "pose": ("enishi_animation_pose_tests", "^enishi_animation_pose_tests$"),
    "cursor": ("enishi_cursor_tests", "^enishi_cursor_tests$"),
    "ecs": ("enishi_ecs_tests", "^enishi_ecs_tests$"),
    "resources": ("renderer_resource_tests", "^renderer_resource_tests$"),
    "script": ("enishi_script_tests", "^enishi_script_tests$"),
    "opengl": ("opengl_shadow_tests", "^opengl_"),
    "vulkan": ("vulkan_render_tests", "^vulkan_render_tests$"),
}
OFFLINE = ["-DFETCHCONTENT_FULLY_DISCONNECTED=ON", "-DFETCHCONTENT_UPDATES_DISCONNECTED=ON",
           "-DVCPKG_MANIFEST_INSTALL=OFF", "-DENISHI_CARGO_OFFLINE=ON"]


def arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__, epilog=(
        "Requires an existing engine build and installed dependencies. Example: "
        "python tools/test_all.py --engine-build build/directx/visualstudio "
        '--generator "Visual Studio 18 2026". Use --suite repeatedly to select suites. '
        "Inactive graphics backends are reported as skipped; failures do not stop other suites."))
    default_engine = "directx/visualstudio" if os.name == "nt" else "opengl/clang"
    parser.add_argument("--engine-build", type=Path, default=ROOT / "build" / default_engine)
    parser.add_argument("--output", type=Path, default=ROOT / "build" / "regression")
    parser.add_argument("--config", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--generator", help="Generator for standalone test builds")
    parser.add_argument("--jobs", type=int, default=min(os.cpu_count() or 2, 4))
    parser.add_argument("--timeout", type=int, default=300, help="Per-test timeout in seconds")
    parser.add_argument("--suite", action="append", choices=tuple(STANDALONE) + tuple(INTEGRATED))
    parser.add_argument("--prefix", action="append", type=Path, default=[], help="Installed dependency prefix")
    parser.add_argument("--define", action="append", default=[], metavar="KEY=VALUE",
                        help="Additional standalone CMake setting, e.g. GLM_INCLUDE_DIR=...")
    parser.add_argument("--rust-toolchain", help="Already installed Rust toolchain for script tests")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without writing files")
    parser.add_argument("--list", action="store_true", help="List suites without running commands")
    args = parser.parse_args()
    if args.jobs < 1 or args.timeout < 1:
        parser.error("--jobs and --timeout must be positive")
    for definition in args.define:
        if "=" not in definition or not definition.split("=", 1)[0]:
            parser.error("--define requires KEY=VALUE")
    args.engine_build = args.engine_build.resolve()
    args.output = args.output.resolve()
    args.suite = list(dict.fromkeys(args.suite or [*STANDALONE, *INTEGRATED]))
    return args


def dependency_prefixes(args: argparse.Namespace) -> list[Path]:
    if args.prefix:
        return [path.resolve() for path in args.prefix]
    vcpkg = os.environ.get("VCPKG_ROOT") or os.environ.get("VCPKG_INSTALLATION_ROOT")
    if not vcpkg:
        executable = shutil.which("vcpkg")
        vcpkg = str(Path(executable).parent) if executable else None
    if vcpkg:
        triplet = os.environ.get("VCPKG_TARGET_TRIPLET", "x64-windows" if os.name == "nt" else "x64-linux")
        return [Path(vcpkg) / "installed" / triplet]
    return []


def environment(prefixes: list[Path], config: str) -> dict[str, str]:
    result = os.environ.copy()
    result.update(CARGO_NET_OFFLINE="true", RUSTUP_AUTO_INSTALL="0")
    bins = []
    for prefix in prefixes:
        if config == "Debug":
            bins.append(str(prefix / "debug" / "bin"))
        bins.append(str(prefix / "bin"))
    result["PATH"] = os.pathsep.join([*bins, result.get("PATH", "")])
    return result


def run_stage(label: str, command: list[str], log: Path, env: dict[str, str], dry_run: bool) -> int:
    print(f"[{label}] {shlex.join(command)}", flush=True)
    if dry_run:
        return 0
    log.parent.mkdir(parents=True, exist_ok=True)
    tail: deque[str] = deque(maxlen=15)
    with log.open("w", encoding="utf-8") as stream:
        stream.write(shlex.join(command) + "\n")
        with subprocess.Popen(command, cwd=ROOT, env=env, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True, errors="replace") as process:
            for line in process.stdout:
                stream.write(line)
                tail.append(line)
            status = process.wait()
    if status != 0:
        print("".join(tail), end="", flush=True)
    print(f"[{label}] {'PASS' if status == 0 else 'FAIL'}; log: {log}", flush=True)
    return status


def test_command(directory: Path, args: argparse.Namespace, pattern: str | None = None) -> list[str]:
    command = ["ctest", "--test-dir", str(directory), "-C", args.config,
               "--output-on-failure", "--no-tests=error", "--timeout", str(args.timeout),
               "--interactive-debug-mode", "0",
               "--parallel", str(args.jobs)]
    if pattern:
        command += ["-R", pattern]
    return command


def standalone_commands(name: str, args: argparse.Namespace, prefixes: list[Path]) -> list[tuple[str, list[str]]]:
    directory = args.output / args.config / name
    configure = ["cmake", "-S", str(ROOT / "src" / "engine" / STANDALONE[name]), "-B", str(directory)]
    if args.generator:
        configure += ["-G", args.generator]
    configure += [f"-DCMAKE_BUILD_TYPE={args.config}",
                  f"-DGLM_INCLUDE_DIR={args.engine_build / '_deps' / 'glm-src'}"]
    if prefixes:
        configure += ["-DCMAKE_PREFIX_PATH=" + ";".join(str(path) for path in prefixes)]
    if name == "vrm":
        configure += [f"-DSTB_INCLUDE_DIR={args.engine_build / 'libs'}"]
    if name == "physics":
        configure += [f"-DBULLET_INCLUDE_DIR={args.engine_build / 'libs' / 'bullet3' / 'src'}",
                      f"-DCMAKE_LIBRARY_PATH={args.engine_build / 'lib' / args.config}"]
    configure += [f"-D{definition}" for definition in args.define] + OFFLINE
    return [("configure", configure),
            ("build", ["cmake", "--build", str(directory), "--config", args.config, "--parallel", str(args.jobs)]),
            ("test", test_command(directory, args))]


def configure_engine(args: argparse.Namespace, env: dict[str, str]) -> tuple[int, set[str]]:
    command = ["cmake", "-S", str(ROOT / "src"), "-B", str(args.engine_build),
               "-DENISHI_CURSOR_TESTS=ON", "-DENISHI_ECS_TESTS=ON", "-DENISHI_SCRIPT_TESTS=ON",
               "-DENISHI_RENDERER_RESOURCE_TESTS=ON", "-DENISHI_OPENGL_TESTS=ON", "-DENISHI_VULKAN_TESTS=ON"]
    if args.rust_toolchain:
        command.append(f"-DENISHI_RUST_TOOLCHAIN={args.rust_toolchain}")
    command += OFFLINE
    status = run_stage("engine/configure", command, args.output / "engine-configure.log", env, args.dry_run)
    if status != 0 or args.dry_run:
        return status, set()
    available = set()
    for directory in (args.engine_build, args.engine_build / "_deps" / "renderer-build"):
        inventory = subprocess.run(["ctest", "--test-dir", str(directory), "-C", args.config,
                                    "--show-only=json-v1"], cwd=ROOT, env=env,
                                   capture_output=True, text=True, errors="replace")
        if inventory.returncode != 0:
            raise RuntimeError(inventory.stderr or inventory.stdout)
        available.update(test["name"] for test in json.loads(inventory.stdout)["tests"])
    return 0, available


def run_suite(name: str, commands: list[tuple[str, list[str]]], args: argparse.Namespace,
              env: dict[str, str]) -> dict[str, object]:
    result: dict[str, object] = {"suite": name, "status": "passed"}
    start = time.monotonic()
    for stage, command in commands:
        try:
            code = run_stage(f"{name}/{stage}", command,
                             args.output / args.config / name / f"{stage}.log", env, args.dry_run)
        except OSError as error:
            code = 1
            result["error"] = str(error)
        if code != 0:
            result.update(status="failed", stage=stage, returncode=code)
            break
    result["seconds"] = round(time.monotonic() - start, 2)
    return result


def main() -> int:
    # Native Windows tools can mix UTF-8 and the console code page in a single build log.
    sys.stdout.reconfigure(errors="backslashreplace")
    sys.stderr.reconfigure(errors="backslashreplace")
    args = arguments()
    if args.list:
        for name in args.suite:
            print(f"{name}: {STANDALONE.get(name, 'engine-integrated')}")
        return 0
    prefixes = dependency_prefixes(args)
    env = environment(prefixes, args.config)
    results = []
    engine_status = None
    available: set[str] = set()
    for name in args.suite:
        if name == "skinning" and os.name != "nt":
            results.append({"suite": name, "status": "skipped", "reason": "DirectX tests require Windows"})
            continue
        if name in STANDALONE:
            commands = standalone_commands(name, args, prefixes)
        else:
            if engine_status is None:
                try:
                    engine_status, available = configure_engine(args, env)
                except (OSError, RuntimeError, ValueError) as error:
                    print(f"Engine configuration failed: {error}", file=sys.stderr)
                    engine_status = 1
            if engine_status != 0:
                results.append({"suite": name, "status": "failed", "stage": "engine-configure"})
                continue
            target, pattern = INTEGRATED[name]
            if not args.dry_run and target not in available:
                status = "skipped" if name in ("opengl", "vulkan") else "failed"
                results.append({"suite": name, "status": status, "reason": "Not registered in the active engine configuration"})
                continue
            test_directory = (args.engine_build / "_deps" / "renderer-build"
                              if name == "resources" else args.engine_build)
            commands = [("build", ["cmake", "--build", str(args.engine_build), "--config", args.config,
                                    "--target", target, "--parallel", str(args.jobs)]),
                        ("test", test_command(test_directory, args, pattern))]
        results.append(run_suite(name, commands, args, env))
    if args.dry_run:
        return 0
    args.output.mkdir(parents=True, exist_ok=True)
    summary = args.output / f"summary-{args.config}.json"
    summary.write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("\nRegression results:")
    for result in results:
        print(f"  {result['suite']}: {result['status']} {result.get('reason', result.get('stage', ''))}")
    failed = sum(result["status"] == "failed" for result in results)
    print(f"Failed: {failed}; summary: {summary}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

"""Regression tests for the CI preflight, without a compiler or graphics device."""

import copy
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

from check_test_inventory import COMMON, CORE, OPENGL, SHADERS, required_tests, validate
from unix_matrix import matrix


@unittest.skipIf(sys.platform == "win32", "macOS SDK shell setup is tested by the Unix CI preflight")
class MacSdkSetupTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="enishi sdk test ")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.sdk = self.root / "enishi-vulkan/1.4.321.0/macOS"
        self.files = ("include/vulkan/vulkan.h", "lib/libvulkan.1.dylib", "lib/libMoltenVK.dylib",
                      "lib/libVkLayer_khronos_validation.dylib",
                      "share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json",
                      "share/vulkan/icd.d/MoltenVK_icd.json", "bin/glslc",
                      "source/SPIRV-Reflect/spirv_reflect.c", "source/SPIRV-Reflect/spirv_reflect.h")
        for name in self.files:
            path = self.sdk / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("fixture", encoding="utf-8")
        (self.sdk / "bin/glslc").chmod(0o755)
        self.environment = {**os.environ, "RUNNER_TEMP": str(self.root),
                            "GITHUB_ENV": str(self.root / "environment"),
                            "GITHUB_PATH": str(self.root / "paths")}

    def run_setup(self):
        script = Path(__file__).with_name("setup_macos_vulkan.sh")
        return subprocess.run(["bash", str(script)], env=self.environment,
                              text=True, capture_output=True, timeout=10)

    def test_cached_sdk_exports_loader_driver_layers_and_sources(self):
        result = self.run_setup()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        environment = (self.root / "environment").read_text(encoding="utf-8")
        for expected in (f"VULKAN_SDK={self.sdk}",
                         f"VK_DRIVER_FILES={self.sdk}/share/vulkan/icd.d/MoltenVK_icd.json",
                         f"VK_LAYER_PATH={self.sdk}/share/vulkan/explicit_layer.d",
                         f"ENISHI_SPIRV_REFLECT_SOURCE_DIR={self.sdk}/source/SPIRV-Reflect"):
            self.assertIn(expected, environment)
        self.assertEqual((self.root / "paths").read_text().strip(), str(self.sdk / "bin"))

    def test_missing_runtime_file_fails_with_its_path(self):
        # Keep the header sentinel so a broken cache cannot start a network download in this test.
        for name in self.files[1:7] + (self.files[8],):
            with self.subTest(name=name):
                path = self.sdk / name
                path.unlink()
                result = self.run_setup()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(str(path), result.stderr)
                self.assertFalse((self.root / "environment").exists())
                path.write_text("fixture", encoding="utf-8")
                if name == "bin/glslc":
                    path.chmod(0o755)

    def test_reflect_source_must_be_unambiguous(self):
        source = self.sdk / "source/SPIRV-Reflect/spirv_reflect.c"
        source.unlink()
        self.assertIn("found 0", self.run_setup().stderr)
        source.write_text("fixture", encoding="utf-8")
        (self.sdk / "duplicate").mkdir()
        (self.sdk / "duplicate/spirv_reflect.c").write_text("fixture", encoding="utf-8")
        result = self.run_setup()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("found 2", result.stderr)
        self.assertFalse((self.root / "environment").exists())


class MatrixTests(unittest.TestCase):
    def test_macos_vulkan_builds_cover_both_architectures_and_configurations(self):
        builds = [entry for entry in matrix()["include"] if entry["os"] == "macos"]
        self.assertEqual(len(builds), 4)
        self.assertEqual({(entry["arch"], entry["config"]) for entry in builds},
                         {("x64", "Debug"), ("x64", "Release"),
                          ("arm64", "Debug"), ("arm64", "Release")})
        for entry in builds:
            self.assertEqual(entry["backend"], "vulkan")
            self.assertEqual(entry["compiler"], "clang")
            self.assertEqual(entry["runner"], "macos-15-intel" if entry["arch"] == "x64" else "macos-15")

    def test_gpu_tests_keep_the_same_build_configuration(self):
        builds = matrix()["include"]
        tests = matrix(tests=True)["include"]
        self.assertEqual(len(builds), len(tests))
        for build, test in zip(builds, tests):
            expected = dict(build)
            if build["os"] == "macos":
                expected["runner"] = "macos-15-xlarge"
            self.assertEqual(test, expected)
        self.assertEqual(len({entry["artifact"] for entry in tests}), len(tests))

    def test_every_matrix_entry_has_a_generated_preset(self):
        repository = Path(__file__).resolve().parents[2]
        presets = json.loads((repository / "src/CMakePresets.json").read_text(encoding="utf-8"))
        names = {entry["name"] for entry in presets["configurePresets"]}
        for entry in matrix(tests=True)["include"]:
            self.assertIn(entry["preset"], names)


class InventoryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.build = Path(self.temporary.name)
        self.executable = self.build / "test-executable"
        self.executable.write_bytes(b"fixture")

    def inventory(self, scope="desktop", backend="vulkan", windows=False):
        tests = [{"name": name, "command": [str(self.executable)],
                  "properties": [{"name": "WORKING_DIRECTORY", "value": str(self.build)}]}
                 for name in sorted(required_tests(scope, backend, windows))]
        if scope == "desktop":
            directory = self.build / "shader" / backend
            directory.mkdir(parents=True, exist_ok=True)
            suffix = "_gl.glsl" if backend == "opengl" else "_vk.spv"
            for name in SHADERS:
                (directory / (name + suffix)).write_bytes(b"shader")
        return {"tests": tests}

    def test_supported_configurations(self):
        for scope in ("common", "core", "desktop"):
            for backend in ("opengl", "vulkan"):
                for windows in (False, True):
                    with self.subTest(scope=scope, backend=backend, windows=windows):
                        self.assertEqual([], validate(self.inventory(scope, backend, windows),
                                                      self.build, scope, backend, windows))

    def test_repository_suite_catalog(self):
        repository = Path(__file__).resolve().parents[2]
        result = subprocess.run(["git", "ls-files", "--", "src/engine/**/CMakeLists.txt"],
                                cwd=repository, check=True, capture_output=True, text=True)
        registered = set()
        for name in result.stdout.splitlines():
            source = (repository / name).read_text(encoding="utf-8")
            names = re.findall(r"add_test\s*\(\s*NAME\s+([^\s)]+)", source)
            for test_name in names:
                self.assertRegex(test_name, r"^[A-Za-z_][A-Za-z_0-9]*$",
                                 f"{name}: declare literal test names so CI can audit every suite")
            registered.update(names)
        covered = COMMON | CORE | OPENGL | {
            "skinning_tests", "shadow_tests", "effect_runtime_tests", "command_parser_tests",
            "vulkan_render_tests", "crack_render_tests",
        }
        self.assertEqual(registered, covered,
                         "Update CI execution and inventory when adding or removing a suite")

    def test_each_missing_registration(self):
        original = self.inventory()
        for index in range(len(original["tests"])):
            inventory = copy.deepcopy(original)
            removed = inventory["tests"].pop(index)["name"]
            with self.subTest(name=removed):
                self.assertIn(removed, "\n".join(validate(inventory, self.build, "desktop", "vulkan", False)))

    def test_missing_executable(self):
        for command in ([], [str(self.build / "missing")]):
            inventory = self.inventory()
            inventory["tests"][0]["command"] = command
            self.assertTrue(validate(inventory, self.build, "desktop", "vulkan", False))

    def test_wrapper_executable_and_script(self):
        inventory = self.inventory()
        wrapper = inventory["tests"][0]
        script = self.build / "check.cmake"
        script.write_text("", encoding="utf-8")
        wrapped = self.build / "diagnostic-executable"
        wrapped.write_bytes(b"fixture")
        wrapper["command"] += [f"-DEXECUTABLE={wrapped}", "-P", str(script)]
        self.assertEqual([], validate(inventory, self.build, "desktop", "vulkan", False))
        wrapped.unlink()
        self.assertIn("wrapped test executable", "\n".join(
            validate(inventory, self.build, "desktop", "vulkan", False)))
        wrapped.write_bytes(b"fixture")
        script.unlink()
        self.assertIn("CMake test script", "\n".join(
            validate(inventory, self.build, "desktop", "vulkan", False)))
        wrapper["command"].pop()
        self.assertIn("CMake test script", "\n".join(
            validate(inventory, self.build, "desktop", "vulkan", False)))

    def test_disabled_and_duplicate(self):
        inventory = self.inventory()
        inventory["tests"][0]["properties"].append({"name": "DISABLED", "value": True})
        self.assertTrue(validate(inventory, self.build, "desktop", "vulkan", False))
        inventory = self.inventory()
        inventory["tests"].append(copy.deepcopy(inventory["tests"][0]))
        self.assertTrue(validate(inventory, self.build, "desktop", "vulkan", False))

    def test_wrong_render_directory(self):
        inventory = self.inventory()
        render = next(test for test in inventory["tests"] if test["name"] == "vulkan_render_tests")
        render["properties"][0]["value"] = str(self.build / "engine/renderer/tests/vulkan")
        self.assertTrue(validate(inventory, self.build, "desktop", "vulkan", False))

    def test_missing_and_empty_shader(self):
        inventory = self.inventory()
        shader = self.build / "shader/vulkan/vs_shadow_map_vk.spv"
        shader.unlink()
        self.assertTrue(validate(inventory, self.build, "desktop", "vulkan", False))
        shader.write_bytes(b"")
        self.assertTrue(validate(inventory, self.build, "desktop", "vulkan", False))


if __name__ == "__main__":
    unittest.main()

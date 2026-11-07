"""Regression tests for the CI preflight, without a compiler or graphics device."""

import copy
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

from check_test_inventory import COMMON, CORE, OPENGL, SHADERS, required_tests, validate


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

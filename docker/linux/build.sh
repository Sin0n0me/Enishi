#!/usr/bin/env bash
set -euo pipefail

backend=${1:-opengl}
configuration=${2:-Debug}
mode=${3:-build}
compiler=${4:-clang}
if [[ $# -gt 4 || ! "$backend" =~ ^(opengl|vulkan)$ ||
      ! "$configuration" =~ ^(Debug|Release)$ || ! "$mode" =~ ^(build|test)$ ||
      ! "$compiler" =~ ^(clang|gcc)$ ]]; then
  echo 'Usage: [opengl|vulkan] [Debug|Release] [build|test] [clang|gcc]' >&2
  exit 2
fi
for required in src/CMakeLists.txt tools/SPIRV-Cross/CMakeLists.txt \
  src/engine/script/Musubu/compiler/musubu_engine/Cargo.toml third_party/tomlplusplus/CMakeLists.txt; do
  if [[ ! -f "$required" ]]; then
    echo "Missing $required; mount the repository at /workspace and initialize its submodules." >&2
    exit 1
  fi
done

while IFS= read -r variable; do
  export "$variable"
done < /opt/enishi/vulkan.env
export PATH="$(cat /opt/enishi/vulkan.path):$PATH"
# Keep Python and Cargo writable without modifying the mounted source tree.
export UV_PROJECT_ENVIRONMENT=/opt/python-tools/.venv
uv sync --locked
export ENISHI_PYTHON="$UV_PROJECT_ENVIRONMENT/bin/python"
export CMAKE_BUILD_PARALLEL_LEVEL=${CMAKE_BUILD_PARALLEL_LEVEL:-3}
cc=clang-19
cxx=clang++-19
if [[ "$compiler" == gcc ]]; then
  cc=gcc-14
  cxx=g++-14
fi
tests=OFF
if [[ "$mode" == test ]]; then
  tests=ON
fi
preset="linux-$backend-$compiler-${configuration,,}"
export BUILD_DIR="/workspace/build/docker/$preset"
cmake --preset "$preset" -S src -B "$BUILD_DIR" \
  "-DCMAKE_C_COMPILER=$cc" "-DCMAKE_CXX_COMPILER=$cxx" \
  "-DPython_EXECUTABLE=$ENISHI_PYTHON" "-DPython3_EXECUTABLE=$ENISHI_PYTHON" \
  -DSDL_WAYLAND=OFF -DSDL_SHARED=OFF -DSDL_STATIC=ON \
  -DENISHI_SHADER_RUNTIME_ROOT=./shader \
  "-DENISHI_SPIRV_REFLECT_SOURCE_DIR=$ENISHI_SPIRV_REFLECT_SOURCE_DIR" \
  "-DENISHI_COMMON_TESTS=$tests" "-DENISHI_EFFECT_RENDER_TESTS=$tests" \
  "-DENISHI_SCRIPT_TESTS=$tests" "-DENISHI_CURSOR_TESTS=$tests" \
  "-DENISHI_ECS_TESTS=$tests" "-DENISHI_RENDERER_RESOURCE_TESTS=$tests" \
  "-DENISHI_OPENGL_TESTS=$([[ "$backend" == opengl ]] && echo "$tests" || echo OFF)" \
  "-DENISHI_VULKAN_TESTS=$([[ "$backend" == vulkan ]] && echo "$tests" || echo OFF)"

if [[ "$mode" == build ]]; then
  cmake --build "$BUILD_DIR" --target enishi_engine enishi-shell
else
  cmake --build "$BUILD_DIR"
  "$ENISHI_PYTHON" -B .github/scripts/check_test_inventory.py "$BUILD_DIR" \
    --config "$configuration" --scope common
  "$ENISHI_PYTHON" -B .github/scripts/check_test_inventory.py "$BUILD_DIR" \
    --config "$configuration" --scope desktop --backend "$backend"
  # Windows checkouts may use CRLF; run a normalized copy without changing sources.
  sed 's/\r$//' .github/scripts/test_x11.sh > "$BUILD_DIR/test_x11.sh"
  xvfb-run -a -s '-screen 0 1280x720x24 +extension GLX +render -noreset' \
    bash "$BUILD_DIR/test_x11.sh"
fi

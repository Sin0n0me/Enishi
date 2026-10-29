#!/usr/bin/env bash
set -euo pipefail

options=("-DCMAKE_CXX_COMPILER=$CXX" "-DCMAKE_BUILD_TYPE=$CONFIGURATION")
if [[ "$RUNNER_OS" == macOS ]]; then
  options+=("-DCMAKE_PREFIX_PATH=$(brew --prefix icu4c)"
    "-DCMAKE_EXE_LINKER_FLAGS=-L$(brew --prefix llvm@21)/lib/c++"
    "-DCMAKE_BUILD_RPATH=$(brew --prefix llvm@21)/lib/c++")
fi

# These targets need neither the application build nor its downloaded renderer dependencies.
for suite in assets_system/tests/vrm_json bypass_shell/tests/command_parser effect/tests/runtime; do
  directory="build/standalone/$suite"
  cmake -S "src/engine/$suite" -B "$directory" -G Ninja "${options[@]}"
  cmake --build "$directory" --parallel 3
  ctest --test-dir "$directory" --output-on-failure --no-tests=error \
    --timeout 120 --output-junit test-results.xml
done

cmake -S src/engine/bypass_shell -B build/standalone/shell -G Ninja "${options[@]}"
cmake --build build/standalone/shell --parallel 3
python3 -B .github/scripts/test_shell_relay.py build/standalone/shell/enishi-shell

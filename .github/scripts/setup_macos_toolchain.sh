#!/usr/bin/env bash
set -euo pipefail

if [[ "$TARGET_ARCH" == x64 ]]; then
  architecture=x86_64
  brew_command=(arch -x86_64 /usr/local/bin/brew)
  if [[ $(uname -m) == arm64 ]]; then
    sudo softwareupdate --install-rosetta --agree-to-license
  fi
  if [[ ! -x /usr/local/bin/brew ]]; then
    installer="$RUNNER_TEMP/install-homebrew.sh"
    curl --fail --location --retry 3 \
      https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh -o "$installer"
    NONINTERACTIVE=1 arch -x86_64 /bin/bash "$installer"
  fi
elif [[ "$TARGET_ARCH" == arm64 ]]; then
  architecture=arm64
  brew_command=(/opt/homebrew/bin/brew)
else
  echo "Unsupported macOS architecture: $TARGET_ARCH" >&2
  exit 1
fi
"${brew_command[@]}" install llvm@21 ninja icu4c
llvm_prefix=$("${brew_command[@]}" --prefix llvm@21)
icu_prefix=$("${brew_command[@]}" --prefix icu4c)
echo "$llvm_prefix/bin" >> "$GITHUB_PATH"
echo "ENISHI_LLVM_PREFIX=$llvm_prefix" >> "$GITHUB_ENV"
echo "ENISHI_ICU_PREFIX=$icu_prefix" >> "$GITHUB_ENV"
echo "ENISHI_OSX_ARCHITECTURE=$architecture" >> "$GITHUB_ENV"

#!/usr/bin/env bash
set -euo pipefail

# Match the existing Windows SDK; headers and SPIRV-Reflect must come from one release.
version=1.4.363.0
checksum=197962f5cbf80baf2775a03336a01cee7c8745686c65aaa70d3f751ade4d7e43
destination="$RUNNER_TEMP/enishi-vulkan"
archive="$RUNNER_TEMP/enishi-vulkan.tar.xz"
if [[ ! -d "$destination/$version" ]]; then
  curl --fail --location --retry 3 \
    "https://sdk.lunarg.com/sdk/download/$version/linux/vulkansdk-linux-x86_64-$version.tar.xz" \
    -o "$archive"
  echo "$checksum  $archive" | sha256sum --check
  mkdir -p "$destination"
  tar -xf "$archive" -C "$destination"
fi
sdk="$destination/$version/x86_64"
test -f "$sdk/include/vulkan/vulkan.h"
test -x "$sdk/bin/glslc"
# SDK releases do not all place bundled sources under a top-level source directory.
# Capture find's status instead of hiding failures inside process substitution.
reflect_list=$(find "$destination/$version" -type f -name spirv_reflect.c -print)
reflect_sources=()
if [[ -n "$reflect_list" ]]; then
  mapfile -t reflect_sources <<< "$reflect_list"
fi
if [[ ${#reflect_sources[@]} != 1 ]]; then
  echo "Expected exactly one spirv_reflect.c under $destination/$version; found ${#reflect_sources[@]}" >&2
  if [[ ${#reflect_sources[@]} != 0 ]]; then
    printf '  %s\n' "${reflect_sources[@]}" >&2
  fi
  exit 1
fi
reflect_directory=$(dirname "${reflect_sources[0]}")
if [[ ! -f "$reflect_directory/spirv_reflect.h" ]]; then
  echo "Missing SPIRV-Reflect header: $reflect_directory/spirv_reflect.h" >&2
  exit 1
fi
echo "VULKAN_SDK=$sdk" >> "$GITHUB_ENV"
echo "ENISHI_SPIRV_REFLECT_SOURCE_DIR=$reflect_directory" >> "$GITHUB_ENV"
echo "LD_LIBRARY_PATH=$sdk/lib/VulkanLoader/lib:$sdk/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" >> "$GITHUB_ENV"
echo "CMAKE_PREFIX_PATH=$sdk:$sdk/lib/VulkanLoader${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}" >> "$GITHUB_ENV"
echo "$sdk/bin" >> "$GITHUB_PATH"

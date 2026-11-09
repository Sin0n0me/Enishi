#!/usr/bin/env bash
set -euo pipefail

# This SDK ships universal Intel/Apple silicon loader, MoltenVK and validation libraries.
version=1.4.321.0
checksum=d873c43acacec1e3330fb530dafd541aa5d8a5726575a98a3f70ca505fc203db
destination="$RUNNER_TEMP/enishi-vulkan/$version"
sdk="$destination/macOS"
if [[ ! -f "$sdk/include/vulkan/vulkan.h" ]]; then
  archive="$RUNNER_TEMP/enishi-vulkan-macos.zip"
  installer_directory="$RUNNER_TEMP/enishi-vulkan-installer"
  curl --fail --location --retry 3 \
    "https://sdk.lunarg.com/sdk/download/$version/mac/vulkansdk-macos-$version.zip" \
    -o "$archive"
  echo "$checksum  $archive" | shasum -a 256 --check
  mkdir -p "$installer_directory"
  ditto -x -k "$archive" "$installer_directory"
  installer="$installer_directory/vulkansdk-macOS-$version.app/Contents/MacOS/vulkansdk-macOS-$version"
  test -x "$installer"
  "$installer" --root "$destination" --accept-licenses --default-answer \
    --confirm-command install copy_only=1
fi

layer_directory="$sdk/share/vulkan/explicit_layer.d"
icd="$sdk/share/vulkan/icd.d/MoltenVK_icd.json"
for required in "$sdk/include/vulkan/vulkan.h" "$sdk/lib/libvulkan.1.dylib" \
  "$sdk/lib/libMoltenVK.dylib" "$sdk/lib/libVkLayer_khronos_validation.dylib" \
  "$layer_directory/VkLayer_khronos_validation.json" "$icd"; do
  if [[ ! -f "$required" ]]; then
    echo "Missing Vulkan SDK file: $required" >&2
    exit 1
  fi
done
if [[ ! -x "$sdk/bin/glslc" ]]; then
  echo "Missing executable shader compiler: $sdk/bin/glslc" >&2
  exit 1
fi

reflect_list=$(find "$destination" -type f -name spirv_reflect.c -print)
reflect_sources=()
while IFS= read -r source; do
  if [[ -n "$source" ]]; then reflect_sources+=("$source"); fi
done <<< "$reflect_list"
if [[ ${#reflect_sources[@]} != 1 ]]; then
  echo "Expected exactly one SDK SPIRV-Reflect source; found ${#reflect_sources[@]}" >&2
  exit 1
fi
reflect_directory=$(dirname "${reflect_sources[0]}")
if [[ ! -f "$reflect_directory/spirv_reflect.h" ]]; then
  echo "Missing SPIRV-Reflect header: $reflect_directory/spirv_reflect.h" >&2
  exit 1
fi

echo "VULKAN_SDK=$sdk" >> "$GITHUB_ENV"
echo "VK_DRIVER_FILES=$icd" >> "$GITHUB_ENV"
echo "VK_LAYER_PATH=$layer_directory" >> "$GITHUB_ENV"
echo "DYLD_LIBRARY_PATH=$sdk/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}" >> "$GITHUB_ENV"
echo "CMAKE_PREFIX_PATH=$sdk${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}" >> "$GITHUB_ENV"
echo "ENISHI_SPIRV_REFLECT_SOURCE_DIR=$reflect_directory" >> "$GITHUB_ENV"
echo "$sdk/bin" >> "$GITHUB_PATH"

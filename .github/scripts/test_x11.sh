#!/usr/bin/env bash
set -euo pipefail

for tool in xprop openbox xcompmgr ctest grep sleep; do
  if ! command -v "$tool" > /dev/null 2>&1; then
    echo "Required X11 test command not found: $tool" >&2
    exit 1
  fi
done

# Run inside xvfb-run so each matrix job owns its display and compositor.
openbox > "$BUILD_DIR/openbox.log" 2>&1 & wm=$!
xcompmgr > "$BUILD_DIR/compositor.log" 2>&1 & compositor=$!
trap 'kill "$wm" "$compositor" 2>/dev/null || true' EXIT
ready=false
for attempt in {1..50}; do
  if xprop -root _NET_SUPPORTING_WM_CHECK | grep -q 'window id'; then
    ready=true
    break
  fi
  sleep 0.1
done
if [[ "$ready" != true ]]; then
  echo 'X11 window manager did not become ready' >&2
  exit 1
fi
ctest --test-dir "$BUILD_DIR" --output-on-failure --no-tests=error \
  --timeout 120 --output-junit test-results.xml

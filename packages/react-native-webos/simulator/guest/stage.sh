#!/usr/bin/env bash
# Replaces the instance's staged app with the resource directory piped in as a tar
# archive: react-native-webos.json, the bundle and its assets. The instance then
# runs the runtime RN_WEBOS_SIMULATOR_RUNTIME names, the build for the app's
# native libraries.
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

runtime="${RN_WEBOS_SIMULATOR_RUNTIME:-}"
if [[ -n "$runtime" && ! "$runtime" =~ $RUNTIME_KEY ]]; then
  echo "Not a runtime key: $runtime" >&2
  exit 2
fi

rm -rf "$APP_DIR"
mkdir -p "$APP_DIR"
tar -x -C "$APP_DIR"
if [[ ! -f "$APP_DIR/react-native-webos.json" ]]; then
  echo "The staged resource directory has no react-native-webos.json." >&2
  exit 1
fi
if [[ -n "$runtime" ]]; then
  echo "$runtime" >"$INSTANCE_DIR/runtime"
else
  rm -f "$INSTANCE_DIR/runtime"
fi

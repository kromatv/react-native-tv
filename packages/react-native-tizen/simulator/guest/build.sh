#!/usr/bin/env bash
# Builds libreact-native-tizen.so and the launcher for the simulator from the
# package's runtime/, into the build of one set of native libraries. The first
# run also builds Hermes' compiler for this VM. One build of a directory runs
# at a time; another waits for it, while other directories build alongside.
#
#   RN_TIZEN_SIMULATOR_RUNTIME  the build's key: rn-tizen derives it from the
#                               app's native libraries (empty: the build from
#                               before keys)
#
# Configures when these are set and differ from the last configure, otherwise
# only runs Ninja, so a host C++ change rebuilds incrementally:
#   RN_TIZEN_CACHE_DIR REACT_NATIVE_DIR RN_TIZEN_CODEGEN_DIR
#   EXPO_MODULES_CORE_DIR EXPO_MODULES_JSI_DIR (both empty for an app without Expo)
#   RN_TIZEN_LIBRARIES_FILE (the app's native libraries; empty for none)
#   BUILD_TYPE (default RelWithDebInfo)
set -euo pipefail
# shellcheck source=SCRIPTDIR/runtimes.sh
source "$(dirname "${BASH_SOURCE[0]}")/runtimes.sh"

RUNTIME_CAP="${RN_TIZEN_SIMULATOR_RUNTIMES:-4}"
RECENT_MINUTES=30

hermes_pin() {
  local field="$1"
  python3 -c 'import json, sys; print(json.load(open(sys.argv[1]))["hermes"][sys.argv[2]])' \
    "$RUNTIME_DIR/dependencies.json" "$field"
}

build_hermes_host() {
  local lock
  exec {lock}>"$HERMES_HOST_LOCK"
  flock "$lock"
  [[ -x "$HERMES_HOST_BUILD_DIR/bin/hermesc" ]] || compile_hermes_host
  exec {lock}>&-
}

compile_hermes_host() {
  local url sha archive_name archive source
  url="$(hermes_pin url)"
  sha="$(hermes_pin sha256)"
  archive_name="hermes-$(basename "$url")"
  archive="$RN_TIZEN_CACHE_DIR/downloads/hermes/$archive_name"
  source="$SOURCE_DIR/${archive_name%.tar.gz}"
  if [[ ! -f "$archive" ]]; then
    mkdir -p "$(dirname "$archive")"
    curl -fL --retry 3 -o "$archive.part" "$url"
    mv "$archive.part" "$archive"
  fi
  echo "$sha  $archive" | sha256sum --check --quiet
  if [[ ! -d "$source" ]]; then
    mkdir -p "$SOURCE_DIR"
    tar -xzf "$archive" -C "$SOURCE_DIR"
  fi
  cmake -S "$source" -B "$HERMES_HOST_BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_LINKER_TYPE=LLD \
    -DHERMES_ENABLE_TEST_SUITE=OFF
  cmake --build "$HERMES_HOST_BUILD_DIR" --target hermesc
}

configure_runtime() {
  local definitions=(
    -DRN_TIZEN_SIMULATOR=ON
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}"
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_LINKER_TYPE=LLD
    -DRN_TIZEN_CACHE_DIR="$RN_TIZEN_CACHE_DIR"
    -DREACT_NATIVE_DIR="$REACT_NATIVE_DIR"
    -DRN_TIZEN_CODEGEN_DIR="$RN_TIZEN_CODEGEN_DIR"
    -DRN_TIZEN_HERMES_HOST_COMPILERS="$HERMES_HOST_BUILD_DIR/ImportHostCompilers.cmake"
    -DEXPO_MODULES_CORE_DIR="${EXPO_MODULES_CORE_DIR:-}"
    -DEXPO_MODULES_JSI_DIR="${EXPO_MODULES_JSI_DIR:-}"
    -DRN_TIZEN_LIBRARIES_FILE="${RN_TIZEN_LIBRARIES_FILE:-}"
  )
  local stamp="$RUNTIME_BUILD_DIR/.rn-tizen-simulator-configure"
  # Checkouts share a build directory per key: without the source tree in the
  # stamp, a build would keep compiling the checkout that configured it first.
  local configured="$RUNTIME_DIR ${definitions[*]}"
  if [[ -f "$RUNTIME_BUILD_DIR/build.ninja" && -f "$stamp" ]] &&
    [[ "$(cat "$stamp")" == "$configured" ]]; then
    return
  fi
  cmake -S "$RUNTIME_DIR" -B "$RUNTIME_BUILD_DIR" -G Ninja "${definitions[@]}"
  printf '%s' "$configured" >"$stamp"
}

key="${RN_TIZEN_SIMULATOR_RUNTIME:-}"
if [[ -n "$key" && ! "$key" =~ $RUNTIME_KEY ]]; then
  echo "Not a runtime key: $key" >&2
  exit 2
fi
mkdir -p "$RUNTIMES_DIR"
RUNTIME_BUILD_DIR="$(runtime_dir "$key")"

exec {build_lock}>"$(runtime_lock "$RUNTIME_BUILD_DIR")"
if ! flock --nonblock "$build_lock"; then
  echo "Waiting for another simulator build to finish..."
  flock "$build_lock"
fi

if [[ -n "${REACT_NATIVE_DIR:-}" ]]; then
  : "${RN_TIZEN_CACHE_DIR:?RN_TIZEN_CACHE_DIR must be set with REACT_NATIVE_DIR}"
  : "${RN_TIZEN_CODEGEN_DIR:?RN_TIZEN_CODEGEN_DIR must be set with REACT_NATIVE_DIR}"
  build_hermes_host
  configure_runtime
elif [[ ! -f "$RUNTIME_BUILD_DIR/build.ninja" ]]; then
  echo "No configured build in $RUNTIME_BUILD_DIR: run \`rn-tizen simulator build\` from the Mac first." >&2
  exit 1
fi

# Every runtime build compiles the same sources from its own directory: hashed
# relative to it, one directory's objects serve the next.
export CCACHE_BASEDIR="$RUNTIME_BUILD_DIR" CCACHE_NOHASHDIR=1
cmake --build "$RUNTIME_BUILD_DIR" --target react-native-tizen rn-tizen-simulator
touch "$RUNTIME_BUILD_DIR/$RUNTIME_USED_FILE"
exec {build_lock}>&-
prune_runtimes "$RUNTIME_CAP" "$RECENT_MINUTES" | sed 's/^/Removed the unused simulator runtime /'

# shellcheck shell=bash disable=SC2034
# Paths and settings every guest script shares: the runtime builds and the
# instances. Sourced, never run.

SIMULATOR_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PACKAGE_DIR="$(dirname "$SIMULATOR_DIR")"
RUNTIME_DIR="$PACKAGE_DIR/runtime"

WORK_DIR="${RN_TIZEN_SIMULATOR_HOME:-$HOME/rn-tizen-simulator}"
SOURCE_DIR="$WORK_DIR/src"
BUILD_DIR="$WORK_DIR/build"
HERMES_HOST_BUILD_DIR="$BUILD_DIR/hermes-host"
HERMES_HOST_LOCK="$BUILD_DIR/hermes-host.lock"
# The one build from before each set of native libraries had its own, which
# an instance staged before then keeps running until it is staged again.
LEGACY_RUNTIME_DIR="$BUILD_DIR/runtime"
LEGACY_BUILD_LOCK="$WORK_DIR/build.lock"
RUNTIMES_DIR="$BUILD_DIR/runtimes"
RUNTIME_KEY='^[0-9a-f]{12}$'
RUNTIME_USED_FILE=".rn-tizen-used"
INSTANCES_DIR="$WORK_DIR/instances"
INSTANCE_NAME='^[a-z0-9][a-z0-9_-]{0,31}$'

SCREEN_SIZE="1920x1080"

mkdir -p "$WORK_DIR"

is_running() {
  local pid_file="$1"
  [[ -f "$pid_file" ]] && kill -0 "$(cat "$pid_file")" 2>/dev/null
}

runtime_dir() {
  local key="$1"
  if [[ -z "$key" ]]; then echo "$LEGACY_RUNTIME_DIR"; else echo "$RUNTIMES_DIR/$key"; fi
}

runtime_lock() {
  local dir="$1"
  if [[ "$dir" == "$LEGACY_RUNTIME_DIR" ]]; then echo "$LEGACY_BUILD_LOCK"; else echo "$dir.lock"; fi
}

launcher_in() {
  local runtime="$1"
  echo "$runtime/simulator/launcher/rn-tizen-simulator"
}

instance_runtime_key() {
  local instance_dir="$1"
  cat "$instance_dir/runtime" 2>/dev/null || true
}

instance_runtime_dir() {
  local instance_dir="$1"
  runtime_dir "$(instance_runtime_key "$instance_dir")"
}

# The command line run.sh starts an instance's app with: systemd-coredump keeps
# it with each core, and it is what tells one instance's crashes from another's.
app_command_line() {
  local instance_dir="$1"
  echo "$(launcher_in "$(instance_runtime_dir "$instance_dir")") $instance_dir/app $instance_dir/control"
}

start_daemon() {
  local pid_file="$1" log="$2"
  shift 2
  if is_running "$pid_file"; then return; fi
  setsid "$@" >"$log" 2>&1 </dev/null &
  echo $! >"$pid_file"
}

stop_daemon() {
  local pid_file="$1" pid
  if ! is_running "$pid_file"; then
    rm -f "$pid_file"
    return 0
  fi
  pid="$(cat "$pid_file")"
  kill "$pid"
  for _ in $(seq 50); do
    kill -0 "$pid" 2>/dev/null || break
    sleep 0.1
  done
  kill -9 "$pid" 2>/dev/null || true
  rm -f "$pid_file"
}

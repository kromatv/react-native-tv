# shellcheck shell=bash disable=SC2034
# One instance: its own screen, window manager, app, staging directory and
# viewer. The CLI names it and derives its display and ports. Sourced, never run.
# shellcheck source=SCRIPTDIR/env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

INSTANCE="${RN_TIZEN_SIMULATOR_INSTANCE:?set by rn-tizen simulator}"
if [[ ! "$INSTANCE" =~ $INSTANCE_NAME ]]; then
  echo "Not an instance name: $INSTANCE" >&2
  exit 2
fi
DISPLAY_NUMBER="${RN_TIZEN_SIMULATOR_DISPLAY:?set by rn-tizen simulator}"
VIEWER_PORT="${RN_TIZEN_SIMULATOR_VIEWER_PORT:?set by rn-tizen simulator}"
VNC_PORT="${RN_TIZEN_SIMULATOR_VNC_PORT:?set by rn-tizen simulator}"
INSTANCE_FLAG=""
[[ "$INSTANCE" == default ]] || INSTANCE_FLAG=" --instance $INSTANCE"

INSTANCE_DIR="$INSTANCES_DIR/$INSTANCE"
RUNTIME_BUILD_DIR="$(instance_runtime_dir "$INSTANCE_DIR")"
LAUNCHER="$(launcher_in "$RUNTIME_BUILD_DIR")"
APP_DIR="$INSTANCE_DIR/app"
APP_LOG="$INSTANCE_DIR/app.log"
APP_PID_FILE="$INSTANCE_DIR/app.pid"
CONTROL_FIFO="$INSTANCE_DIR/control"
DISPLAY_FILE="$INSTANCE_DIR/display"
XVFB_PID_FILE="$INSTANCE_DIR/xvfb.pid"
WM_PID_FILE="$INSTANCE_DIR/wm.pid"
VNC_PID_FILE="$INSTANCE_DIR/vnc.pid"
VIEWER_PID_FILE="$INSTANCE_DIR/viewer.pid"
VIEWER_WEB_DIR="$INSTANCE_DIR/viewer"

export DISPLAY=":$DISPLAY_NUMBER"
export ELM_DISPLAY=x11
export ELM_ACCEL=none
export RN_TIZEN_SIMULATOR_DATA_DIR="$INSTANCE_DIR/data"
export RN_TIZEN_SIMULATOR_CACHE_DIR="$INSTANCE_DIR/cache"
export RN_TIZEN_SIMULATOR_RESOURCE_DIR="$APP_DIR"
export EINA_LOG_BACKTRACE=-1

mkdir -p "$INSTANCE_DIR"

require_app() {
  if ! is_running "$APP_PID_FILE"; then
    echo "No app is running in instance $INSTANCE: start one with \`rn-tizen simulator run$INSTANCE_FLAG\`." >&2
    exit 1
  fi
}

app_window() {
  require_app
  xdotool search --onlyvisible --pid "$(cat "$APP_PID_FILE")" 2>/dev/null | head -n 1 || true
}

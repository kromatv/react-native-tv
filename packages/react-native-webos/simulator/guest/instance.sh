# shellcheck shell=bash disable=SC2034
# One instance: its own screen, window manager, app, staging directory and
# viewer. The CLI names it and derives its display and ports. Sourced, never run.
# shellcheck source=SCRIPTDIR/env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

INSTANCE="${RN_WEBOS_SIMULATOR_INSTANCE:?set by rn-webos simulator}"
if [[ ! "$INSTANCE" =~ $INSTANCE_NAME ]]; then
  echo "Not an instance name: $INSTANCE" >&2
  exit 2
fi
DISPLAY_NUMBER="${RN_WEBOS_SIMULATOR_DISPLAY:?set by rn-webos simulator}"
VIEWER_PORT="${RN_WEBOS_SIMULATOR_VIEWER_PORT:?set by rn-webos simulator}"
VNC_PORT="${RN_WEBOS_SIMULATOR_VNC_PORT:?set by rn-webos simulator}"
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
SCREEN_SIZE_FILE="$INSTANCE_DIR/screen-size"
XVFB_PID_FILE="$INSTANCE_DIR/xvfb.pid"
WM_PID_FILE="$INSTANCE_DIR/wm.pid"
VNC_PID_FILE="$INSTANCE_DIR/vnc.pid"
VIEWER_PID_FILE="$INSTANCE_DIR/viewer.pid"
VIEWER_WEB_DIR="$INSTANCE_DIR/viewer"

export DISPLAY=":$DISPLAY_NUMBER"
export SDL_VIDEODRIVER=x11
export RN_WEBOS_RESOURCE_DIR="$APP_DIR"
export RN_WEBOS_APP_DATA_DIR="$INSTANCE_DIR/appdata"
export RN_WEBOS_SIMULATOR_CONTROL="$CONTROL_FIFO"
export RN_WEBOS_MODEL_NAME="Simulator"
export RN_WEBOS_PLATFORM_VERSION="simulator"

mkdir -p "$INSTANCE_DIR"

require_app() {
  if ! is_running "$APP_PID_FILE"; then
    echo "No app is running in instance $INSTANCE: start one with \`rn-webos simulator run$INSTANCE_FLAG\`." >&2
    exit 1
  fi
}

app_window() {
  require_app
  xdotool search --onlyvisible --pid "$(cat "$APP_PID_FILE")" 2>/dev/null | head -n 1 || true
}

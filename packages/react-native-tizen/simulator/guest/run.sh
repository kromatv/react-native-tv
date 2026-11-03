#!/usr/bin/env bash
# (Re)starts the instance's staged app on its own virtual 1920x1080 screen and
# returns once its window is up. The app's output goes to $APP_LOG, a crash to
# systemd-coredump. Every other instance keeps running.
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

display_owner() {
  local dir
  for dir in "$INSTANCES_DIR"/*; do
    [[ "$dir" == "$INSTANCE_DIR" ]] && continue
    if is_running "$dir/xvfb.pid" && [[ "$(cat "$dir/display" 2>/dev/null)" == "$DISPLAY" ]]; then
      basename "$dir"
      return
    fi
  done
  echo "a program outside the simulator"
}

wait_for_display() {
  for _ in $(seq 50); do
    xdpyinfo >/dev/null 2>&1 && return
    sleep 0.1
  done
  echo "Xvfb did not start; see $INSTANCE_DIR/xvfb.log" >&2
  exit 1
}

start_display() {
  if ! is_running "$XVFB_PID_FILE"; then
    if xdpyinfo >/dev/null 2>&1; then
      echo "Display $DISPLAY of instance $INSTANCE is taken by $(display_owner): give this instance another name." >&2
      exit 1
    fi
    start_daemon "$XVFB_PID_FILE" "$INSTANCE_DIR/xvfb.log" \
      Xvfb "$DISPLAY" -screen 0 "${SCREEN_SIZE}x24" -nolisten tcp
    echo "$DISPLAY" >"$DISPLAY_FILE"
    wait_for_display
  fi
  # Elementary stops drawing a window no window manager has shown (it stays
  # "withdrawn"); a TV's compositor shows and focuses the app the same way.
  start_daemon "$WM_PID_FILE" "$INSTANCE_DIR/wm.log" \
    matchbox-window-manager -use_titlebar no -use_cursor no
}

wait_for_window() {
  for _ in $(seq 300); do
    if ! is_running "$APP_PID_FILE"; then
      echo "The app exited during startup:" >&2
      tail -n 40 "$APP_LOG" >&2
      exit 1
    fi
    local window
    window="$(app_window)"
    if [[ -n "$window" ]]; then
      xdotool windowfocus --sync "$window"
      return
    fi
    sleep 0.1
  done
  echo "The app opened no window in 30 s; see $APP_LOG" >&2
  exit 1
}

if [[ ! -x "$LAUNCHER" ]]; then
  echo "No launcher at $LAUNCHER: run \`rn-tizen simulator run$INSTANCE_FLAG\` from the app first." >&2
  exit 1
fi
touch "$RUNTIME_BUILD_DIR/$RUNTIME_USED_FILE"
if [[ ! -f "$APP_DIR/react-native-tizen.json" ]]; then
  echo "No app is staged in instance $INSTANCE: run \`rn-tizen simulator run$INSTANCE_FLAG\` from the app first." >&2
  exit 1
fi

stop_daemon "$APP_PID_FILE"
start_display
rm -f "$CONTROL_FIFO"
mkfifo -m 600 "$CONTROL_FIFO"
ulimit -c unlimited
start_daemon "$APP_PID_FILE" "$APP_LOG" "$LAUNCHER" "$APP_DIR" "$CONTROL_FIFO"
wait_for_window
cat "$APP_PID_FILE"

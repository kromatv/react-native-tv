#!/usr/bin/env bash
# Serves the instance's live screen to a browser: x11vnc shares its virtual
# screen and websockify serves viewer/ with noVNC on $VIEWER_PORT, which Lima
# forwards to the Mac's localhost. Repeating it leaves a running viewer alone.
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

wait_for_port() {
  local port="$1" log_name="$2"
  for _ in $(seq 50); do
    ss -ltn "sport = :$port" | grep -q LISTEN && return
    sleep 0.1
  done
  echo "Nothing listens on port $port; see $INSTANCE_DIR/$log_name.log" >&2
  exit 1
}

if ! is_running "$XVFB_PID_FILE"; then
  echo "The virtual screen of instance $INSTANCE is not running: start the app with \`rn-tizen simulator run$INSTANCE_FLAG\`." >&2
  exit 1
fi

mkdir -p "$VIEWER_WEB_DIR"
ln -sfn "$SIMULATOR_DIR/viewer/index.html" "$VIEWER_WEB_DIR/index.html"
ln -sfn "$SIMULATOR_DIR/viewer/viewer.js" "$VIEWER_WEB_DIR/viewer.js"
ln -sfn /usr/share/novnc/core "$VIEWER_WEB_DIR/core"
ln -sfn /usr/share/novnc/vendor "$VIEWER_WEB_DIR/vendor"

start_daemon "$VNC_PID_FILE" "$INSTANCE_DIR/vnc.log" \
  x11vnc -display "$DISPLAY" -localhost -rfbport "$VNC_PORT" -forever -shared -nopw \
  -add_keysyms -quiet
wait_for_port "$VNC_PORT" vnc
start_daemon "$VIEWER_PID_FILE" "$INSTANCE_DIR/viewer.log" \
  websockify --web "$VIEWER_WEB_DIR" "127.0.0.1:$VIEWER_PORT" "127.0.0.1:$VNC_PORT"
wait_for_port "$VIEWER_PORT" viewer

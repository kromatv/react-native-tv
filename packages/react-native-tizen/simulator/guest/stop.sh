#!/usr/bin/env bash
# Stops one instance: its app, its viewer, and its virtual screen with its
# window manager. Every other instance keeps running.
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

for pid_file in "$APP_PID_FILE" "$VIEWER_PID_FILE" "$VNC_PID_FILE" "$WM_PID_FILE" "$XVFB_PID_FILE"; do
  stop_daemon "$pid_file"
done

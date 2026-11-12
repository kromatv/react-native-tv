#!/usr/bin/env bash
# Presses remote keys on the running app, in order. A remote button name
# becomes the X keysym a Samsung remote sends; anything else is passed to
# xdotool as a keysym, e.g. Escape.
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

keysym() {
  local key="$1"
  case "${key,,}" in
    left | right | up | down) echo "${key^}" ;;
    enter | ok | select | return) echo Return ;;
    back) echo XF86Back ;;
    exit) echo XF86Exit ;;
    info) echo XF86Info ;;
    play) echo XF86AudioPlay ;;
    pause) echo XF86AudioPause ;;
    playpause) echo XF86AudioPlayPause ;;
    stop) echo XF86AudioStop ;;
    rewind) echo XF86AudioRewind ;;
    fastforward) echo XF86AudioNext ;;
    previous) echo XF86AudioPrev ;;
    red | green | yellow | blue) echo "XF86${key^}" ;;
    *) echo "$key" ;;
  esac
}

if [[ $# -eq 0 ]]; then
  echo "usage: key.sh <key>..." >&2
  exit 2
fi
window="$(app_window)"
if [[ -z "$window" ]]; then
  echo "The app has no window yet." >&2
  exit 1
fi
xdotool windowfocus --sync "$window"
for key in "$@"; do
  xdotool key --clearmodifiers "$(keysym "$key")"
  sleep 0.05
done

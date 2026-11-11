#!/usr/bin/env bash
# Writes one command to the control channel of the instance's running app: a
# lifecycle change (pause, resume, exit), a deep link (open-url <url>), a
# launch of the running app with new parameters (relaunch [<json>]), the
# remote's pointer (pointer-move <x> <y>, pointer-down, pointer-up,
# pointer-wheel <notches>, pointer-leave), or what the TV's on-screen keyboard
# does to a focused text field (text <text>, compose [<text>], keyboard
# shown|hidden). The app's side is runtime/host/src/platform/SimulatorControl.h.
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

if [[ $# -eq 0 ]]; then
  echo "usage: control.sh <command> [<argument>...]" >&2
  exit 2
fi
require_app
timeout 2 tee "$CONTROL_FIFO" <<<"$*" >/dev/null

#!/usr/bin/env bash
# Sends one lifecycle command to the instance's running app: pause, resume, exit
# or open-url <url> (a deep link, as the TV delivers it).
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

case "${1:-}" in
  pause | resume | exit) command="$1" ;;
  open-url) command="open-url ${2:?usage: control.sh open-url <url>}" ;;
  *)
    echo "usage: control.sh pause|resume|exit|open-url <url>" >&2
    exit 2
    ;;
esac
require_app
timeout 2 tee "$CONTROL_FIFO" <<<"$command" >/dev/null

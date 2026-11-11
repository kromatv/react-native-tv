#!/usr/bin/env bash
# Sends one lifecycle command to the instance's running app: pause, resume, exit,
# open-url <url> (a deep link, as the TV delivers it) or relaunch [<json>] (the
# parameters of a launch of the running app).
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

# The command and its argument may come as one word or as two.
read -r name argument <<<"$*"
case "$name" in
  pause | resume | exit) command="$name" ;;
  open-url) command="open-url ${argument:?usage: control.sh open-url <url>}" ;;
  relaunch) command="relaunch $argument" ;;
  *)
    echo "usage: control.sh pause|resume|exit|open-url <url>|relaunch [<json>]" >&2
    exit 2
    ;;
esac
require_app
timeout 2 tee "$CONTROL_FIFO" <<<"$command" >/dev/null

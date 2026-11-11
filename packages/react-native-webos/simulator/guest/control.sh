#!/usr/bin/env bash
# Sends one command to the instance's running app: a lifecycle change (pause,
# resume, exit), open-url <url> (a deep link, as the TV delivers it), or what
# the TV's on-screen keyboard does to a focused text field: text <text> (it
# commits that text), compose [<text>] (it shows that text as its composition;
# no text ends the composition), keyboard shown|hidden.
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

action="${1:-}"
shift || true
case "$action" in
  pause | resume | exit) command="$action" ;;
  open-url) command="open-url ${1:?usage: control.sh open-url <url>}" ;;
  text) command="text ${*:?usage: control.sh text <text>}" ;;
  compose) command="compose $*" ;;
  keyboard)
    case "${1:-}" in
      shown | hidden) command="keyboard $1" ;;
      *)
        echo "usage: control.sh keyboard shown|hidden" >&2
        exit 2
        ;;
    esac
    ;;
  *)
    echo "usage: control.sh pause|resume|exit|open-url <url>|text <text>|compose [<text>]|keyboard shown|hidden" >&2
    exit 2
    ;;
esac
require_app
timeout 2 tee "$CONTROL_FIFO" <<<"$command" >/dev/null

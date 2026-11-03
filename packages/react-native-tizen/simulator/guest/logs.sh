#!/usr/bin/env bash
# Prints the instance's app log: the last N lines (default 200), then follows it with -f.
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

lines=200
follow=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    -f | --follow) follow=(-F) ;;
    -n | --lines) lines="$2"; shift ;;
    *) echo "usage: logs.sh [-f] [-n <lines>]" >&2; exit 2 ;;
  esac
  shift
done
touch "$APP_LOG"
exec tail -n "$lines" "${follow[@]}" "$APP_LOG"

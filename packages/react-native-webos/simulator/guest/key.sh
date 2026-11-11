#!/usr/bin/env bash
# Presses remote keys on the running app, in order. Each goes to the app as the
# key event the TV's remote sends, through its control FIFO, so a key takes the
# same path as on a TV. `<key>:<milliseconds>` holds the key that long.
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

if [[ $# -eq 0 ]]; then
  echo "usage: key.sh <key>[:<milliseconds>]..." >&2
  exit 2
fi
require_app
for key in "$@"; do
  name="${key%%:*}"
  hold=""
  [[ "$key" == *:* ]] && hold="${key#*:}"
  if [[ -n "$hold" ]]; then
    if [[ ! "$hold" =~ ^[0-9]+$ ]]; then
      echo "Not a hold in milliseconds: $key" >&2
      exit 2
    fi
    timeout 2 tee "$CONTROL_FIFO" <<<"key-hold $name $hold" >/dev/null
    sleep "$(awk -v ms="$hold" 'BEGIN { printf "%.3f", ms / 1000 + 0.1 }')"
  else
    timeout 2 tee "$CONTROL_FIFO" <<<"key $name" >/dev/null
    sleep 0.12
  fi
done

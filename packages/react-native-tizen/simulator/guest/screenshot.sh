#!/usr/bin/env bash
# Writes the instance's screen as a PNG to stdout.
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

if ! is_running "$XVFB_PID_FILE"; then
  echo "The virtual screen of instance $INSTANCE is not running: start an app with \`rn-tizen simulator run$INSTANCE_FLAG\`." >&2
  exit 1
fi
import -window root png:-

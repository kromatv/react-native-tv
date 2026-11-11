#!/usr/bin/env bash
# Prints the symbolized backtrace of every thread at the last crash of the
# instance's app, from the core systemd-coredump kept.
set -euo pipefail
# shellcheck source=SCRIPTDIR/instance.sh
source "$(dirname "${BASH_SOURCE[0]}")/instance.sh"

core="$INSTANCE_DIR/core"
rm -f "$core"
match="COREDUMP_CMDLINE=$(app_command_line "$INSTANCE_DIR")"
if ! sudo coredumpctl --no-pager --quiet dump -1 "$match" -o "$core" >/dev/null 2>&1; then
  echo "The app in instance $INSTANCE has not crashed."
  exit 0
fi
sudo chown "$(id -u)" "$core"
gdb -batch -q -iex 'set debuginfod enabled off' -ex 'set print thread-events off' \
  -ex 'thread apply all bt 30' "$LAUNCHER" "$core" 2>&1 |
  grep -v -e '^\[New LWP' -e '^warning:' -e '^$' -e 'libthread_db' || true
rm -f "$core"

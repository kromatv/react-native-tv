#!/usr/bin/env bash
# Prints the simulator's state as one JSON object: every runtime build, then
# every instance the VM holds with the runtime it runs.
set -euo pipefail
# shellcheck source=SCRIPTDIR/runtimes.sh
source "$(dirname "${BASH_SOURCE[0]}")/runtimes.sh"

pid_or_null() {
  if is_running "$1"; then cat "$1"; else echo null; fi
}

app_crashed() {
  local dir="$1" pid="$2"
  ! is_running "$dir/app.pid" &&
    coredumpctl --no-pager --quiet list "COREDUMP_CMDLINE=$(app_command_line "$dir")" 2>/dev/null |
    awk -v pid="$pid" '$5 == pid { found = 1 } END { exit !found }'
}

is_built() {
  if [[ -x "$(launcher_in "$1")" ]]; then echo true; else echo false; fi
}

instance_json() {
  local dir="$1" last_pid=null crashed=false staged=false runtime
  if [[ -f "$dir/app.pid" ]]; then
    last_pid="$(cat "$dir/app.pid")"
    app_crashed "$dir" "$last_pid" && crashed=true
  fi
  [[ -f "$dir/app/react-native-tizen.json" ]] && staged=true
  runtime="$(instance_runtime_dir "$dir")"
  printf '{"name":"%s","staged":%s,"app":%s,"lastApp":%s,"crashed":%s,"screen":%s,"viewer":%s,"runtime":"%s","runtimeBuilt":%s}' \
    "$(basename "$dir")" "$staged" "$(pid_or_null "$dir/app.pid")" "$last_pid" "$crashed" \
    "$(pid_or_null "$dir/xvfb.pid")" "$(pid_or_null "$dir/viewer.pid")" \
    "$(runtime_key_of "$runtime")" "$(is_built "$runtime")"
}

runtime_json() {
  printf '{"key":"%s","built":%s}' "$(runtime_key_of "$1")" "$(is_built "$1")"
}

runtimes=()
while read -r dir; do
  runtimes+=("$(runtime_json "$dir")")
done < <(runtime_dirs)
instances=()
for dir in "$INSTANCES_DIR"/*; do
  [[ -d "$dir" && "$(basename "$dir")" =~ $INSTANCE_NAME ]] || continue
  instances+=("$(instance_json "$dir")")
done

printf '{"runtimes":[%s],"instances":[%s]}\n' \
  "$(IFS=,; echo "${runtimes[*]}")" "$(IFS=,; echo "${instances[*]}")"

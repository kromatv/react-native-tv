# shellcheck shell=bash
# The runtime builds: one per set of native libraries an app compiles in,
# listed, and removed when no instance runs them. Sourced, never run.
# shellcheck source=SCRIPTDIR/env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

runtime_dirs() {
  local dir
  [[ -d "$LEGACY_RUNTIME_DIR" ]] && echo "$LEGACY_RUNTIME_DIR"
  for dir in "$RUNTIMES_DIR"/*; do
    [[ -d "$dir" && "$(basename "$dir")" =~ $RUNTIME_KEY ]] && echo "$dir"
  done
  return 0
}

runtime_key_of() {
  [[ "$1" == "$LEGACY_RUNTIME_DIR" ]] || basename "$1"
}

runtime_in_use() {
  local dir
  for dir in "$INSTANCES_DIR"/*; do
    [[ -d "$dir" && "$(instance_runtime_dir "$dir")" == "$1" ]] && return 0
  done
  return 1
}

runtimes_by_last_use() {
  local dir
  while read -r dir; do
    printf '%s %s\n' "$(stat -c %Y "$dir/$RUNTIME_USED_FILE" 2>/dev/null || echo 0)" "$dir"
  done < <(runtime_dirs) | sort -rn | cut -d ' ' -f 2-
}

remove_runtime() {
  local dir="$1" lock
  exec {lock}>"$(runtime_lock "$dir")"
  if ! flock --nonblock "$lock"; then
    exec {lock}>&-
    return 1
  fi
  rm -rf "$dir"
  [[ "$dir" == "$LEGACY_RUNTIME_DIR" ]] || rm -f "$dir.lock"
  exec {lock}>&-
}

# Removes, least recently used first, every runtime build past the newest
# $1 that no instance runs and none is building; $2 spares builds used within
# that many minutes. Prints each key it removed.
prune_runtimes() {
  local keep="$1" spare_minutes="$2" index=0 dir
  while read -r dir; do
    index=$((index + 1))
    ((index > keep)) || continue
    runtime_in_use "$dir" && continue
    if ((spare_minutes > 0)) && [[ -n "$(find "$dir/$RUNTIME_USED_FILE" -mmin "-$spare_minutes" 2>/dev/null)" ]]; then
      continue
    fi
    local key
    key="$(runtime_key_of "$dir")"
    remove_runtime "$dir" && echo "${key:-runtime}"
  done < <(runtimes_by_last_use)
  return 0
}

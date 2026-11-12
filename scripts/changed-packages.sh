#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'USAGE'
Usage: BASE=<commit> scripts/changed-packages.sh

Prints, for each directory in packages/, two lines in the form GitHub Actions
reads as outputs:

  <package>=true|false         a file of the package changed since BASE
  <package>-native=true|false  one of them is an input of its native runtime,
                               as the package's scripts/native-inputs.ts decides

Without BASE, or when a file every package builds from changed, every line
says true.
USAGE
  exit 2
}

[[ $# -eq 0 ]] || usage
cd "$(dirname "${BASH_SOURCE[0]}")/.."

shared=(
  package.json
  bun.lock
  scripts/changed-packages.sh
  .github/workflows/ci.yml
  .github/workflows/package.yml
)

all=false
if [[ -z ${BASE:-} ]] || ! git diff --quiet "$BASE" HEAD -- "${shared[@]}"; then
  all=true
fi

for directory in packages/*/; do
  package=$(basename "$directory")
  has_native=false
  [[ -f $directory/scripts/native-inputs.ts ]] && has_native=true

  if [[ $all == true ]]; then
    echo "$package=true"
    echo "$package-native=$has_native"
    continue
  fi

  files=$(git -C "$directory" diff --name-only --relative "$BASE" HEAD)
  if [[ -z $files ]]; then
    echo "$package=false"
    echo "$package-native=false"
    continue
  fi
  echo "$package=true"
  if [[ $has_native == true ]]; then
    (cd "$directory" && bun scripts/native-inputs.ts <<<"$files") | sed "s/^native=/$package-native=/"
  else
    echo "$package-native=false"
  fi
done

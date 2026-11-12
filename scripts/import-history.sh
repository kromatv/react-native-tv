#!/usr/bin/env bash
set -euo pipefail

owner=kromatv
remote_base=${IMPORT_REMOTE_BASE:-https://github.com/$owner}

usage() {
  cat >&2 <<'USAGE'
Usage: scripts/import-history.sh <package>...
       scripts/import-history.sh --rewrite-only <directory> <package>...

Clones github.com/kromatv/<package>, rewrites its history with git filter-repo
and merges it into the branch checked out here:

  - every path moves under packages/<package>/
  - every tag gets <package>- as a prefix
  - #123 in a commit message becomes kromatv/<package>#123
  - authors, committers and dates stay as they are

The rewrite gives the same commit hashes on every run, so a later run merges
only the commits the old repository gained since the last one.

--rewrite-only leaves the rewritten clones in <directory> and merges nothing.
IMPORT_REMOTE_BASE replaces https://github.com/kromatv as the place to clone from.
USAGE
  exit 2
}

die() {
  echo "import-history: $*" >&2
  exit 1
}

rewrite() {
  local package=$1 clone=$2
  git clone --quiet --no-local "$remote_base/$package.git" "$clone"
  IMPORT_REPOSITORY="$owner/$package" git -C "$clone" filter-repo \
    --quiet \
    --to-subdirectory-filter "packages/$package" \
    --tag-rename ":$package-" \
    --prune-empty never \
    --prune-degenerate never \
    --message-callback '
import os, re
repository = os.environb[b"IMPORT_REPOSITORY"]
return re.sub(
    rb"(\S+://\S+)|(?<![\w/.\x80-\xff-])#([0-9]+)\b",
    lambda found: found.group(1) or repository + b"#" + found.group(2),
    message,
)'
}

merge_rewritten() {
  local package=$1 clone=$2 tip last_import imported title
  local -a merge_options=(--no-ff)
  tip=$(git -C "$clone" rev-parse --verify refs/heads/main)
  last_import=$(git -C "$root" log --merges --format=%H -1 \
    --grep="^chore: import $package with its history\$" \
    --grep="^chore: import the new commits of $package\$")

  if [[ -n $last_import ]]; then
    imported=$(git -C "$root" rev-parse --verify "$last_import^2")
    git -C "$clone" merge-base --is-ancestor "$imported" "$tip" 2>/dev/null ||
      die "$package: the rewrite does not contain $imported, the commit imported last time. Nothing was merged."
    title="chore: import the new commits of $package"
  else
    git -C "$root" cat-file -e "HEAD:packages/$package" 2>/dev/null &&
      die "packages/$package exists, but no commit here imported it."
    merge_options+=(--allow-unrelated-histories)
    title="chore: import $package with its history"
  fi

  git -C "$root" fetch --quiet --no-tags "$clone" \
    refs/heads/main "refs/tags/$package-*:refs/tags/$package-*"
  if git -C "$root" merge-base --is-ancestor "$tip" HEAD; then
    echo "$package: nothing new since the last import"
    return
  fi
  git -C "$root" merge --quiet "${merge_options[@]}" -m "$title" "$tip" ||
    die "$package: resolve the conflicts and commit the merge as \"$title\", then run this again for the packages left."
  echo "$package: merged $(git -C "$root" rev-list --count "HEAD^1..$tip") commits, up to $tip"
}

rewrite_only=
if [[ ${1:-} == --rewrite-only ]]; then
  [[ $# -ge 3 ]] || usage
  rewrite_only=$2
  shift 2
fi
[[ $# -ge 1 ]] || usage
for package in "$@"; do
  [[ $package =~ ^[a-z0-9][a-z0-9-]*$ ]] || usage
done
git filter-repo --version >/dev/null 2>&1 || die "git filter-repo is not installed."

if [[ -n $rewrite_only ]]; then
  mkdir -p "$rewrite_only"
  for package in "$@"; do
    rewrite "$package" "$rewrite_only/$package"
  done
  exit
fi

root=$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)
[[ -z $(git -C "$root" status --porcelain --untracked-files=no) ]] ||
  die "commit or stash the changes in $root first."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

for package in "$@"; do
  rewrite "$package" "$work/$package"
  merge_rewritten "$package" "$work/$package"
done

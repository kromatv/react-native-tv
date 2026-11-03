#!/bin/sh
# usage: scripts/quality/ub-mode.sh [coverage] [crap] [dry] [mutate] [summary]
# With no step, runs coverage, crap, dry and mutate. See scripts/quality/README.md.
set -eu

CRAPPER_SHA=6fd92c4c9bcc56efb64a2f8743d5a2993bcfd42d
DRYER_SHA=66ff6d21a42c04afcad89c78a80066176d1294b0
MUTATOR_SHA=a5a0cb803b66e0cdb8bf471b3b08af701d424a41

cd "$(git rev-parse --show-toplevel)"
TOOLS="${UB_MODE_HOME:-$PWD/.cache/ub-mode}"
PYTHON="${UB_MODE_PYTHON:-python3}"
OUT=.metrics/ub-mode
STEPS="${*:-coverage crap dry mutate}"

install_tool() {
  name="$1"
  sha="$2"
  dir="$TOOLS/$name"
  if [ ! -d "$dir" ]; then
    git clone --quiet "https://github.com/unclebob/$name" "$dir"
    git -C "$dir" checkout --quiet "$sha"
  fi
  if [ ! -x "$dir/.venv/bin/python" ]; then
    "$PYTHON" -m venv "$dir/.venv"
    "$dir/.venv/bin/pip" install --quiet -e "$dir"
  fi
}

ts_sources() {
  git ls-files 'src/*.ts' | grep -vE '\.test\.ts$|\.fixtures\.ts$|\.d\.ts$'
}

coverage() {
  rm -rf coverage
  bun run test --coverage > "$OUT/coverage.txt" 2>&1
  tail -6 "$OUT/coverage.txt"
}

crap() {
  # shellcheck disable=SC2046
  "$TOOLS/crapper/crapper" --use-existing-coverage $(ts_sources) > "$OUT/crap.txt"
  head -25 "$OUT/crap.txt"
}

dry() {
  # shellcheck disable=SC2046
  "$TOOLS/dryer/dryer" $(ts_sources) > "$OUT/dry.txt"
  tail -5 "$OUT/dry.txt"
}

mutate() {
  status=0
  # shellcheck disable=SC2046
  "$TOOLS/mutator/mutator" --use-existing-coverage --mutate-all --timeout-factor 30 \
    --test-command "node_modules/.bin/vitest run --bail=1" $(ts_sources) > "$OUT/mutate.txt" 2>&1 || status=$?
  grep -E '^Total' "$OUT/mutate.txt" | tail -1
  if [ "$status" -ne 0 ] && [ "$status" -ne 3 ]; then
    tail -20 "$OUT/mutate.txt" >&2
    return "$status"
  fi
}

code_block() {
  printf '%s\n' '~~~'
  cat
  printf '%s\n\n' '~~~'
}

summary() {
  printf '## Mutation testing\n\n'
  grep -E '^Total' "$OUT/mutate.txt" | tail -1 | code_block
  printf '<details><summary>%s surviving mutants</summary>\n\n' "$(grep -c '^SURVIVED' "$OUT/mutate.txt" || true)"
  grep '^SURVIVED' "$OUT/mutate.txt" | code_block
  printf '</details>\n\n## CRAP, worst 25\n\n'
  sed -n '5,31p' "$OUT/crap.txt" | code_block
  printf '## Structural duplicates\n\n'
  code_block < "$OUT/dry.txt"
}

"$PYTHON" -c 'import sys; sys.exit(sys.version_info < (3, 11))' || {
  echo "The ub-mode tools need Python 3.11 or newer: set UB_MODE_PYTHON to one." >&2
  exit 1
}
mkdir -p "$OUT" "$TOOLS"
install_tool crapper "$CRAPPER_SHA"
install_tool dryer "$DRYER_SHA"
install_tool mutator "$MUTATOR_SHA"
for step in $STEPS; do
  case "$step" in
    coverage | crap | dry | mutate | summary) "$step" ;;
    *)
      echo "Unknown step $step: use coverage, crap, dry, mutate or summary." >&2
      exit 2
      ;;
  esac
done

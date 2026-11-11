#!/usr/bin/env bash
# Removes every runtime build no instance runs and none is building, and
# prints the key of each. An instance keeps its runtime until it is staged on
# another, so `restart` still finds it.
set -euo pipefail
# shellcheck source=SCRIPTDIR/runtimes.sh
source "$(dirname "${BASH_SOURCE[0]}")/runtimes.sh"

prune_runtimes 0 0

#!/usr/bin/env bash
# Installs what the simulator builds and runs with. Run as root inside the VM;
# it does nothing until this file changes.
set -euo pipefail

stamp=/var/lib/rn-tizen-simulator/provisioned
fingerprint="$(sha256sum "${BASH_SOURCE[0]}" | cut -d ' ' -f 1)"
if [[ -f "$stamp" && "$(cat "$stamp")" == "$fingerprint" ]]; then exit 0; fi

export DEBIAN_FRONTEND=noninteractive

packages=(
  clang lld cmake ninja-build pkg-config ccache git python3 gdb
  libefl-all-dev libevas1-engines-x libcurl4-openssl-dev
  xvfb matchbox-window-manager xdotool x11-utils imagemagick
  x11vnc novnc websockify systemd-coredump
  fonts-dejavu-core fonts-noto-core
)

apt-get update -q
apt-get install -y -q --no-install-recommends "${packages[@]}"
mkdir -p "$(dirname "$stamp")"
echo "$fingerprint" >"$stamp"

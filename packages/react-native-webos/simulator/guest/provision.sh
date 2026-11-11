#!/usr/bin/env bash
# Installs what the simulator builds and runs with. Run as root inside the VM;
# it does nothing until this file changes.
set -euo pipefail

stamp=/var/lib/rn-webos-simulator/provisioned
fingerprint="$(sha256sum "${BASH_SOURCE[0]}" | cut -d ' ' -f 1)"
if [[ -f "$stamp" && "$(cat "$stamp")" == "$fingerprint" ]]; then exit 0; fi

export DEBIAN_FRONTEND=noninteractive

# Skia is built with GN (generate-ninja); SDL's X11 backend needs the X
# headers, and Mesa's llvmpipe stands in for the TV's OpenGL ES driver. ICU is
# for Hermes' compiler, which runs in the VM; the runtime compiles its own.
packages=(
  clang lld cmake ninja-build generate-ninja pkg-config ccache git python3 gdb curl unzip
  libegl-dev libgles-dev libgl1-mesa-dri libcurl4-openssl-dev libicu-dev
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev
  libxtst-dev libxkbcommon-dev
  xvfb matchbox-window-manager xdotool x11-utils imagemagick
  x11vnc novnc websockify systemd-coredump
  fonts-dejavu-core fonts-noto-core
)

apt-get update -q
apt-get install -y -q --no-install-recommends "${packages[@]}"
mkdir -p "$(dirname "$stamp")"
echo "$fingerprint" >"$stamp"

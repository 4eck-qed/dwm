#!/bin/sh
set -e

DIR="$(cd -- "$(dirname -- "$0")" && pwd)"
cd "$DIR"

mkdir -p build
make clean || true
make -j "$(nproc)"
sudo make install

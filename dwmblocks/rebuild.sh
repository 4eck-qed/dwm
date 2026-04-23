#!/bin/sh

set -e

rm blocks.h
make
sudo make install

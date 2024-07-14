#!/bin/sh

mkdir build &>/dev/null

set -e

exe="build/$(basename -s .cpp "$1")"

flags="$(pkg-config osmesa libpng zlib --libs --cflags)"

if [ ! -e "$exe" ] || [ "$1" -nt "$exe" ]; then
    clang++ -g -pipe -std=c++23 $flags "$1" -o "$exe"
fi

./"$exe" ${@:2}

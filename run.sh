#!/bin/sh

set -e

SFML_DIR=/home/rno/dev/sfml-install
CLANG_DIR=/home/rno/dev/clang-install

export LD_LIBRARY_PATH="$CLANG_DIR/lib:$SFML_DIR/lib"

cd $(dirname $1)
./$(basename $1)

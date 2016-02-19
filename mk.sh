#!/bin/sh

set -e

BUILD_TYPE=Release
CXX_STD=1z
SFML_DIR=/home/runpac/dev/sfml-install

CXX_FLAGS="-stdlib=libc++ -std=c++1z -I/usr/include/c++/v1"
CXX_LINKER_FLAGS="-stdlib=libc++ -L/usr/lib -lc++ -lc++abi"

export CXX=/usr/bin/clang++
export CC=/usr/bin/clang

CTEST_OUTPUT_ON_FAILURE=1
export CTEST_OUTPUT_ON_FAILURE

rm -rf build
mkdir build
cd build
cmake -DRAKO_CXX_STD=$CXX_STD -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DRAKO_SFML_DIR=$SFML_DIR \
  -DCMAKE_CXX_FLAGS="${CXX_FLAGS}" -DCMAKE_EXE_LINKER_FLAGS="${CXX_LINKER_FLAGS}" \
  ..
make
make test
#ARGS="-V"
cd ..

#valgrind build/test/packed_array
#valgrind build/test/entity_manager
#valgrind build/test/perf

#./build/test/packed_array
#./build/test/entity_manager
#./build/test/perf

#./build/test/experimental2
#./build/test/bench

#./build/test/entity_group_manager

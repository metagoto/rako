#!/bin/sh

set -e

BUILD_TYPE=Release
CXX_STD=1z
SFML_DIR=/home/rno/dev/sfml-install

CLANG_DIR=/home/rno/dev/clang-install
LIBCPP_INC="$CLANG_DIR/include/c++/v1"
LIBCPP_LIB="$CLANG_DIR/lib"

CXX_FLAGS="-stdlib=libc++ -std=c++1z -I$LIBCPP_INC"
CXX_LINKER_FLAGS="-stdlib=libc++ -L$LIBCPP_LIB -lc++ -lc++abi"

export CXX="$CLANG_DIR/bin/clang++"
export CC="$CLANG_DIR/bin/clang"
export LD_LIBRARY_PATH="$CLANG_DIR/lib"

CTEST_OUTPUT_ON_FAILURE=1
export CTEST_OUTPUT_ON_FAILURE

rm -rf build
mkdir build
cd build
cmake -DRAKO_CXX_STD=$CXX_STD -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DRAKO_SFML_DIR=$SFML_DIR \
  -DCMAKE_CXX_FLAGS="${CXX_FLAGS}" -DCMAKE_EXE_LINKER_FLAGS="${CXX_LINKER_FLAGS}" \
  ..
make

mkdir example/media
cp -r ../example/media example

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

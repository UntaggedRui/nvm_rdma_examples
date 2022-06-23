#!/bin/bash

if [[ -d build ]]; then
  echo "Remove existing build directory";
  rm -rf build
fi
echo "Create build directory";
mkdir build
cd build
cmake ..
make -j$(nproc)

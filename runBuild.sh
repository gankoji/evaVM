#!/bin/zsh

cmake . -B build
cmake --build build
cd build && ctest && cd ..

./build/eva-vm
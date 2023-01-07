#!/bin/zsh

cmake . -B build
cmake --build build
cd build && ctest && cd ..

./build/eva-vm -e '(var x 10) x'
./build/eva-vm -f ./test/test.eva
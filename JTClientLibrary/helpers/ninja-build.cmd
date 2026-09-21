@echo off

call "%VS80COMNTOOLS%vsvars32.bat"

mkdir -p cmake-ninja-build/

cmake -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo -B cmake-ninja-build/ -S .
cmake --build cmake-ninja-build/ -- -j 1


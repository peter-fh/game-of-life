#!/bin/bash

mkdir -p build
cd build
cmake ..
if make; then
	cd ..
	./build/LearnGL $1
else
	cd ..
fi



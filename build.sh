#!/bin/bash

mkdir -p build
cd build
cmake ..
if make; then
	cd ..
	./build/LearnGL $@
else
	cd ..
fi


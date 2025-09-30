#!/bin/bash

mkdir -p build
cd build
cmake ..
if make; then
	cd ..
	./build/Game\ Of\ Life $@
else
	cd ..
fi


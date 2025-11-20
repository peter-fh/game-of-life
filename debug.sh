#!/bin/bash

cmake --preset debug
if cmake --build ./build/debug; then
	./build/debug/Game\ Of\ Life "$@"
fi


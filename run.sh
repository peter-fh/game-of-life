#!/bin/bash

cmake --preset default
if cmake --build ./build/default; then
	./build/default/Game\ Of\ Life "$@"
fi


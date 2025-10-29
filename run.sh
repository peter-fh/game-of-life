#!/bin/bash


cmake --preset default
if cmake --build ./build; then
	./build/Game\ Of\ Life "$@"
fi


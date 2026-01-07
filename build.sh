#!/bin/bash
cmake --preset=debug -S . -B build -DBUILD_GODOT=OFF -DBUILD_CUTE=ON
cmake --build build


#!/bin/bash
cmake --preset=debug -S . -B build -DBUILD_GODOT=ON -DBUILD_CUTE=ON
cmake --build build


#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPOSITORY_DIR="$SCRIPT_DIR/.."

cmake -S ./CitySimulator -B ./.cmake/cs -DCMAKE_BUILD_TYPE=Release  # Ninja, don`t work config debug/release
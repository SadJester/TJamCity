#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPOSITORY_DIR="$SCRIPT_DIR/.."

if [ ! -d "${REPOSITORY_DIR}/CitySimulator" ]; then
    echo "Directory CitySimulator not found"
    exit 1
fi

find ./CitySimulator -type f -exec clang-format -i {} \;

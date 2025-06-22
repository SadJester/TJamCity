#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TARGET_DIR="${SCRIPT_DIR}/bin"
mkdir -p "${TARGET_DIR}"

# Determine OS
unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     os='linux64';;
    Darwin*)    os='macos';;
    CYGWIN*|MINGW*) os='win64';;
    *)          os='';;
esac

if [ -z "$os" ]; then
    echo "Unsupported operating system: ${unameOut}"
    exit 1
fi

VERSION=${1:-0.10}
ARCHIVE="tracy-${VERSION}-${os}"
URL="https://github.com/wolfpld/tracy/releases/download/v${VERSION}/${ARCHIVE}.zip"

echo "Downloading ${URL}..."

curl -L "$URL" -o "${TARGET_DIR}/${ARCHIVE}.zip"

unzip -o "${TARGET_DIR}/${ARCHIVE}.zip" -d "${TARGET_DIR}"
rm "${TARGET_DIR}/${ARCHIVE}.zip"

echo "Tracy downloaded to ${TARGET_DIR}"

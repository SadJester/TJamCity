#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TARGET_DIR="${SCRIPT_DIR}/bin/tracy"
mkdir -p "${TARGET_DIR}"

# Determine OS and executable
unameOut="$(uname -s)"
case "${unameOut}" in
    CYGWIN*|MINGW*)
        os='windows'
        executable='tracy-profiler.exe'
        ;;
    *)
        echo "Unsupported operating system: ${unameOut}. Only Windows is supported for now."
        exit 1
        ;;
esac

VERSION="0.12.1"
ARCHIVE="tracy-${VERSION}-${os}"

get_tracy_url() {
    local version=$1
    local os_type=$2
    local archive="${os_type}-${version}"
    echo "https://github.com/wolfpld/tracy/releases/download/v${version}/${archive}.zip"
}

URL=$(get_tracy_url "$VERSION" "$os")

echo "Downloading ${URL}..."

EXECUTABLE_PATH="${TARGET_DIR}/${executable}"

if [ ! -f "${EXECUTABLE_PATH}" ]; then
	curl -L "$URL" -o "${TARGET_DIR}/${ARCHIVE}.zip" --ssl-no-revoke
	unzip -o "${TARGET_DIR}/${ARCHIVE}.zip" -d "${TARGET_DIR}"
  	rm "${TARGET_DIR}/${ARCHIVE}.zip"
	echo "Tracy downloaded to ${TARGET_DIR}"
else
    echo "Tracy already exists in ${TARGET_DIR}"
fi


"${EXECUTABLE_PATH}"

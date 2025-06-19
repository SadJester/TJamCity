#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPOSITORY_DIR="$SCRIPT_DIR/.."

if [ ! -d "${REPOSITORY_DIR}/CitySimulator" ]; then
    echo "Directory CitySimulator not found"
    exit 1
fi

cd "$REPOSITORY_DIR" || exit 1

# Collect staged and changed (but unstaged) .cpp and .h files in CitySimulator
staged_files=$(git diff --staged --name-only)
unstaged_files=$(git diff --name-only)

all_files=$(echo -e "${staged_files}\n${unstaged_files}" | sort -u | grep -E '^CitySimulator/.*\.(cpp|h)$')

if [ -z "$all_files" ]; then
    echo "No changed or staged .cpp/.h files found in CitySimulator."
    exit 0
fi

# Format all collected files
while IFS= read -r file; do
    clang-format -i "$file"
    echo "$file"
done <<< "$all_files"

echo "Done"

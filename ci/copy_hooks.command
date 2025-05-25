#!/usr/bin/env bash

# ------- CONFIG -------
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPOSITORY_DIR="$SCRIPT_DIR/.."

HOOKS_SOURCE_DIR="${REPOSITORY_DIR}/ci/hooks"
HOOKS_TARGET_DIR="${REPOSITORY_DIR}/.git/hooks"
# ----------------------

# Check if source directory exists
if [ ! -d "$HOOKS_SOURCE_DIR" ]; then
  echo "Error: Source directory '$HOOKS_SOURCE_DIR' not found!" >&2
  exit 1
fi

# Create target directory if it doesn't exist
mkdir -p "$HOOKS_TARGET_DIR"
if [ $? -ne 0 ]; then
  echo "Error: Failed to create target directory '$HOOKS_TARGET_DIR'" >&2
  exit 1
fi

# Copy each file and set executability
for hook_file in "$HOOKS_SOURCE_DIR"/*; do
  if [ -f "$hook_file" ]; then  # Only process files
    filename=$(basename "$hook_file")
    target_path="$HOOKS_TARGET_DIR/$filename"
    
    # Copy file
    cp -v "$hook_file" "$target_path"
    
    # Make executable (ignore errors on Windows/WSL)
    chmod +x "$target_path" 2>/dev/null || true
    
    echo "Installed hook: $filename"
  fi
done

echo "All hooks copied successfully to $HOOKS_TARGET_DIR"
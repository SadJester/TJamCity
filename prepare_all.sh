#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPOSITORY_DIR="$SCRIPT_DIR"

# Функция для определения операционной системы
detect_os() {
    unameOut="$(uname -s)"
    case "${unameOut}" in
        Linux*)     os='linux';;
        Darwin*)    os='mac';;
        CYGWIN*)    os='windows';;
        MINGW*)     os='windows';;
        *)          os='unknown';;
    esac
}

# Основная функция
main() {
    detect_os
    
    case $os in
        windows)
            # Проверяем существование команды choco
            if command -v choco > /dev/null 2>&1; then
                echo "Installing clang-format via Chocolatey..."
                # choco install llvm -y
                # echo "Installation completed successfully!"
                echo "Need to investigate what is happening and install llvm"
            else
                echo "Chocolatey not found. Please install Chocolatey first:"
                echo "1. Open PowerShell as Administrator"
                echo "2. Run: Set-ExecutionPolicy Bypass -Scope Process -Force"
                echo "3. Run: iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"
                echo "4. Restart your command prompt"
                exit 1
            fi
            ;;
        mac)
            echo "Installing clang-format via Homebrew..."
            brew install clang-format
            ;;
        *)
            echo "Unsupported operating system: $os"
            exit 1
            ;;
    esac
}

# Запуск основной функции
main


${REPOSITORY_DIR}/ci/copy_hooks.command

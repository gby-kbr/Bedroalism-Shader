#!/bin/bash
# Build tool for Linux, macOS and Termux: ./build.sh setup | mats | pack

command -v python3 >/dev/null 2>&1 || { echo "Python not found." >&2; exit 1; }

LD_LIBRARY_PATH=./tool/lib python3 tool "$@"

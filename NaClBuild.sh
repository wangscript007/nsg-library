#!/bin/bash
set -e
export SOURCE_FOLDER="$PWD"

if [ ! -n "$NACL_SDK_ROOT" ]; then
    echo "Environment variable NACL_SDK_ROOT shall be set."
    exit 0
fi

if [ ! -n "$1" ]; then
	echo "Enter build target directory. (directory will be created on the parent directory of the current one"
	exit 0
fi

cd ..

if [ ! -d "$1" ]; then
	mkdir $1
fi

cd $1

cmake $SOURCE_FOLDER -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="$SOURCE_FOLDER/PNaCl.Linux.cmake"
make
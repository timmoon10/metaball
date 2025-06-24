#!/bin/bash
set -ex

# Paths
: ${SRC_DIR:=$(dirname $(realpath $0))}
pushd ${SRC_DIR}

# Use Homebrew Clang on OSX
if [ "$(uname)" == "Darwin" ]; then
    export PATH=${PATH}:/opt/homebrew/opt/llvm/bin
fi

# clang-format
clang-format -style=Google -i src/metaball/* include/metaball/* include/util/* include/util/impl/*

# Clean up
popd

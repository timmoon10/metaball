#!/bin/bash
set -ex

# Paths
: ${SRC_DIR:=$(dirname $(realpath $0))}
: ${BUILD_DIR:=${SRC_DIR}/build}

# Create directories
rm -rf ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR}

# Construct CMake invocation
CMAKE_COMMAND="cmake
-GNinja
-DCMAKE_BUILD_TYPE=Release
-DCMAKE_VERBOSE_MAKEFILE=ON
-S ${SRC_DIR}
-B ${BUILD_DIR}
"
if [ "$(uname)" == "Darwin" ]; then
    # Use Homebrew Clang on OSX
    OPENMP_PATH=${OPENMP_PATH:=/opt/homebrew/opt/libomp}
    CMAKE_COMMAND="${CMAKE_COMMAND}
-DOpenMP_CXX_FLAGS=\"-Xpreprocessor -fopenmp -I${OPENMP_PATH}/include\"
-DOpenMP_CXX_LIB_NAMES=omp
-DOpenMP_omp_LIBRARY=${OPENMP_PATH}/lib/libomp.a
"
fi
if [ -n "${QT_PATH}" ]; then
    # Add QT path if provided
    CMAKE_COMMAND="${CMAKE_COMMAND} -DCMAKE_PREFIX_PATH=${QT_PATH}/lib/cmake"
fi
CMAKE_COMMAND=$(echo "${CMAKE_COMMAND}" | tr '\n' ' ')

# Build application
bash -c "${CMAKE_COMMAND}"
cmake --build ${BUILD_DIR}

# Clean up
popd

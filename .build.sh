#!/bin/sh
mkdir build
cd build
set -ev
cmake -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="MOS6502" -DLLVM_USE_LINKER="lld" -DLLVM_ENABLE_PROJECTS="clang" -G "Ninja" ../llvm
ninja check-all

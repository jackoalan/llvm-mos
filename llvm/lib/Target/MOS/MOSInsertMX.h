//===-- MOSInsertMX.h - MOS MX Insertion ------------------------*- C++ -*-===//
//
// Part of LLVM-MOS, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the MOS MX insertion pass.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MOS_MOSINSERTMX_H
#define LLVM_LIB_TARGET_MOS_MOSINSERTMX_H

#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {

MachineFunctionPass *createMOSInsertMXPass();

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS_MOSINSERTMX_H

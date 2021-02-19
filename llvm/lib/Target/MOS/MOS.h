//===-- MOS.h - Top-level interface for MOS representation ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// MOS back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MOS_MOS_H
#define LLVM_LIB_TARGET_MOS_MOS_H

#include "llvm/Pass.h"

namespace llvm {

void initializeMOSCombinerPass(PassRegistry &);
void initializeMOSIndexIVPass(PassRegistry &);
void initializeMOSNoRecursePass(PassRegistry &);
void initializeMOSPreRegAllocPass(PassRegistry &);
void initializeMOSStaticStackAllocPass(PassRegistry &);

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS_MOS_H
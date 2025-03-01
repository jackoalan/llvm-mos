//===-- MOSMCAsmInfo.h - MOS asm properties ---------------------*- C++ -*-===//
//
// Part of LLVM-MOS, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the MOSMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MOS_ASM_INFO_H
#define LLVM_MOS_ASM_INFO_H

#include "llvm/MC/MCAsmInfo.h"

namespace llvm {

class Triple;

/// Specifies the format of MOS assembly files.
class MOSMCAsmInfo : public MCAsmInfo {
public:
  explicit MOSMCAsmInfo(const Triple &TT, const MCTargetOptions &Options);
};

} // end namespace llvm

#endif // LLVM_MOS_ASM_INFO_H

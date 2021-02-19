//===-- MOSMachineFuctionInfo.h - MOS machine function info -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares MOS-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIB_TARGET_MOS_MOSMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_MOS_MOSMACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

class MOSFunctionInfo : public MachineFunctionInfo {
  int VarArgsStackIndex;

public:
  MOSFunctionInfo(MachineFunction& MF) {}

  int getVarArgsStackIndex() const { return VarArgsStackIndex; }
  void setVarArgsStackIndex(int Index) { VarArgsStackIndex = Index; }
};

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS_MOSMACHINEFUNCTIONINFO_H

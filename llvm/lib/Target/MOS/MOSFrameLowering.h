//===-- MOSFrameLowering.h - Define frame lowering for MOS ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MOS_MOSFRAMELOWERING_H
#define LLVM_LIB_TARGET_MOS_MOSFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class MOSFrameLowering : public TargetFrameLowering {
public:
  MOSFrameLowering();

  bool enableShrinkWrapping(const MachineFunction &MF) const override {
    return true;
  }

  void processFunctionBeforeFrameFinalized(
      MachineFunction &MF, RegScavenger *RS = nullptr) const override;

  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MI) const override;

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  bool hasFP(const MachineFunction &MF) const override;

  bool isSupportedStackID(TargetStackID::Value ID) const override;

  // Computes the size of the hard stack.
  uint64_t hsSize(const MachineFrameInfo &MFI) const;

  // Computes the size of the static stack.
  uint64_t staticSize(const MachineFrameInfo &MFI) const;
};

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS_MOSFRAMELOWERING_H

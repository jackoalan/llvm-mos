#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502FRAMELOWERING_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502FRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class MOS6502FrameLowering : public TargetFrameLowering {
public:
  MOS6502FrameLowering();

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

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502FRAMELOWERING_H

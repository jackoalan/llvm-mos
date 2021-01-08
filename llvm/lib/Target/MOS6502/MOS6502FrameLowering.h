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

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  bool hasFP(const MachineFunction &MF) const override;

  // Computes the size of the hard stack.
  uint64_t hsSize(const MachineFrameInfo &MFI) const;
};

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502FRAMELOWERING_H

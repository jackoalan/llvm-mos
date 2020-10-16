#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502FRAMELOWERING_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502FRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class MOS6502FrameLowering : public TargetFrameLowering {
 public:
  MOS6502FrameLowering();

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  bool hasFP(const MachineFunction &MF) const override;
};

}  // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502FRAMELOWERING_H

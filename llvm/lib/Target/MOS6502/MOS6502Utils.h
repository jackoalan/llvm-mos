#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502UTILS_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502UTILS_H

#include "MCTargetDesc/MOS6502MCTargetDesc.h"

#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineBasicBlock.h"

namespace llvm {

// Ensures that any instructions emitted in Body are free to clobber NZ by
// saving and restoring it around the region if necessary.
template <typename T>
void mos6502WithNZFree(MachineIRBuilder &Builder, const T &Body) {
  const MachineBasicBlock &MBB = Builder.getMBB();
  if (MBB.computeRegisterLiveness(
          MBB.getParent()->getSubtarget().getRegisterInfo(), MOS6502::NZ,
          Builder.getInsertPt()) != MachineBasicBlock::LQR_Dead) {
    // Ensure that push and pull are both in place before body so that NZ is
    // known dead within the region. That way, calls to this function in the
    // body won't save again.
    Builder.buildInstr(MOS6502::PHP)->getOperand(0).setIsKill();
    // The insertion point is right before pull.
    Builder.setInstr(*Builder.buildInstr(MOS6502::PLP));
  }

  Body();
}

// Ensures that any instructions emitted in Body are free to clobber A by saving
// and restoring it around the region if necessary. Note that this does not save
// NZ, and if A needs to be saved/restored, this will clobber NZ.
template <typename T>
void mos6502WithAFree(MachineIRBuilder &Builder, const T &Body) {
  const MachineBasicBlock &MBB = Builder.getMBB();
  if (MBB.computeRegisterLiveness(
          MBB.getParent()->getSubtarget().getRegisterInfo(), MOS6502::A,
          Builder.getInsertPt()) != MachineBasicBlock::LQR_Dead) {
    // Ensure that push and pull are both in place before body so that A is
    // known dead within the region. That way, calls to this function in the
    // body won't save again.
    Builder.buildInstr(MOS6502::PHA)->getOperand(0).setIsKill();
    // The insertion point is right before pull.
    Builder.setInstr(*Builder.buildInstr(MOS6502::PLA));
  }

  Body();
}

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502UTILS_H

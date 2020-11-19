#include "MOS6502InstrInfo.h"
#include "MOS6502RegisterInfo.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "MOS6502GenInstrInfo.inc"

void MOS6502InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   const DebugLoc &DL, MCRegister DestReg,
                                   MCRegister SrcReg, bool KillSrc) const {
  if (!MOS6502::GPRRegClass.contains(DestReg) ||
      !MOS6502::GPRRegClass.contains(SrcReg)) {
    report_fatal_error("Unsupported physical register copy.");
  }
  if (SrcReg == MOS6502::A) {
    assert(MOS6502::XYRegClass.contains(DestReg));
    MachineIRBuilder Builder(MBB, MI);
    Builder.buildInstr(MOS6502::TA_).addUse(DestReg);
  } else if (DestReg == MOS6502::A) {
    assert(MOS6502::XYRegClass.contains(SrcReg));
    MachineIRBuilder Builder(MBB, MI);
    Builder.buildInstr(MOS6502::T_A).addUse(SrcReg);
  } else {
    MachineIRBuilder Builder(MBB, MI);
    Builder.buildInstr(MOS6502::PHA);
    copyPhysReg(MBB, Builder.getInsertPt(), Builder.getDebugLoc(), MOS6502::A, SrcReg, KillSrc);
    copyPhysReg(MBB, Builder.getInsertPt(), Builder.getDebugLoc(), DestReg, MOS6502::A, true);
    Builder.buildInstr(MOS6502::PLA);
  }
}

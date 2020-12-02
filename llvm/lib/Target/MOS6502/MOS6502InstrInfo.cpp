#include "MOS6502InstrInfo.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502RegisterInfo.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "MOS6502GenInstrInfo.inc"

void MOS6502InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   const DebugLoc &DL, MCRegister DestReg,
                                   MCRegister SrcReg, bool KillSrc) const {
  MachineIRBuilder Builder(MBB, MI);

  const auto &areClasses = [&](const TargetRegisterClass &Src,
                               const TargetRegisterClass &Dest) {
    return Src.contains(SrcReg) && Dest.contains(DestReg);
  };

  bool preserveP = MBB.computeRegisterLiveness(
                       MBB.getParent()->getSubtarget().getRegisterInfo(),
                       MOS6502::NZ, MI) != MachineBasicBlock::LQR_Dead;

  if (preserveP)
    Builder.buildInstr(MOS6502::PHP);

  if (areClasses(MOS6502::GPRRegClass, MOS6502::GPRRegClass)) {
    if (SrcReg == MOS6502::A) {
      assert(MOS6502::XYRegClass.contains(DestReg));
      Builder.buildInstr(MOS6502::TA_).addUse(DestReg);
    } else if (DestReg == MOS6502::A) {
      assert(MOS6502::XYRegClass.contains(SrcReg));
      Builder.buildInstr(MOS6502::T_A).addUse(SrcReg);
    } else {
      copyPhysReg(MBB, Builder.getInsertPt(), Builder.getDebugLoc(), MOS6502::A,
                  SrcReg, KillSrc);
      copyPhysReg(MBB, Builder.getInsertPt(), Builder.getDebugLoc(), DestReg,
                  MOS6502::A, true);
      Builder.buildInstr(MOS6502::PLA);
    }
  } else if (areClasses(MOS6502::GPRRegClass, MOS6502::ZPRegClass)) {
    Builder.buildInstr(MOS6502::STzp).addDef(DestReg).addUse(SrcReg);
  } else if (areClasses(MOS6502::ZPRegClass, MOS6502::GPRRegClass)) {
    Builder.buildInstr(MOS6502::LDzp).addDef(DestReg).addUse(SrcReg);
  } else {
    report_fatal_error("Unsupported physical register copy.");
  }

  if (preserveP)
    Builder.buildInstr(MOS6502::PLP);
}

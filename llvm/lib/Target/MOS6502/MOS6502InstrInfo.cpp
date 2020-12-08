#include "MOS6502InstrInfo.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502RegisterInfo.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "MOS6502GenInstrInfo.inc"

static bool maybeLive(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                      MCRegister Reg) {
  return MBB.computeRegisterLiveness(
             MBB.getParent()->getSubtarget().getRegisterInfo(), Reg, MI) !=
         MachineBasicBlock::LQR_Dead;
}

void MOS6502InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   const DebugLoc &DL, MCRegister DestReg,
                                   MCRegister SrcReg, bool KillSrc) const {
  MachineIRBuilder Builder(MBB, MI);

  const auto &areClasses = [&](const TargetRegisterClass &Src,
                               const TargetRegisterClass &Dest) {
    return Src.contains(SrcReg) && Dest.contains(DestReg);
  };

  bool nzMaybeLive = maybeLive(MBB, MI, MOS6502::NZ);

  if (nzMaybeLive)
    Builder.buildInstr(MOS6502::PHP);

  if (areClasses(MOS6502::GPRRegClass, MOS6502::GPRRegClass)) {
    if (SrcReg == MOS6502::A) {
      assert(MOS6502::XYRegClass.contains(DestReg));
      Builder.buildInstr(MOS6502::TA_).addDef(DestReg);
    } else if (DestReg == MOS6502::A) {
      assert(MOS6502::XYRegClass.contains(SrcReg));
      Builder.buildInstr(MOS6502::T_A).addUse(SrcReg);
    } else {
      bool aMaybeLive = maybeLive(MBB, MI, MOS6502::A);

      if (aMaybeLive)
        Builder.buildInstr(MOS6502::PHA);
      copyPhysReg(MBB, Builder.getInsertPt(), Builder.getDebugLoc(), MOS6502::A,
                  SrcReg, KillSrc);
      copyPhysReg(MBB, Builder.getInsertPt(), Builder.getDebugLoc(), DestReg,
                  MOS6502::A, true);
      if (aMaybeLive)
        Builder.buildInstr(MOS6502::PLA);
    }
  } else if (areClasses(MOS6502::GPRRegClass, MOS6502::ZPRegClass)) {
    Builder.buildInstr(MOS6502::STzp).addDef(DestReg).addUse(SrcReg);
  } else if (areClasses(MOS6502::ZPRegClass, MOS6502::GPRRegClass)) {
    Builder.buildInstr(MOS6502::LDzp).addDef(DestReg).addUse(SrcReg);
  } else {
    report_fatal_error("Unsupported physical register copy.");
  }

  if (nzMaybeLive)
    Builder.buildInstr(MOS6502::PLP);
}

std::pair<unsigned, unsigned>
MOS6502InstrInfo::decomposeMachineOperandsTargetFlags(unsigned TF) const {
  return std::make_pair(TF, 0u);
}

ArrayRef<std::pair<unsigned, const char *>>
MOS6502InstrInfo::getSerializableDirectMachineOperandTargetFlags() const {
  static const std::pair<unsigned, const char *> Flags[] = {
      {MOS6502::MO_LO, "lo"}, {MOS6502::MO_HI, "hi"}};
  return Flags;
}

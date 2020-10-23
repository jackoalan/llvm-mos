#include "MOS6502CallLowering.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

bool MOS6502CallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                      const Value *Val,
                                      ArrayRef<Register> VRegs) const {
  if (!Val) return true;

  MachineInstrBuilder Instr = MIRBuilder.buildInstr(MOS6502::RETURN);
  for (const Register& VReg : VRegs) {
    Instr.addUse(VReg);
  }

  return true;
}

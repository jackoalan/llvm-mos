#include "MOS6502InstructionSelector.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502RegisterInfo.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/RegisterBankInfo.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;
using namespace MIPatternMatch;

bool MOS6502InstructionSelector::select(MachineInstr &I) {
  switch (I.getOpcode()) {
  case MOS6502::RTS:
    return true;
  }

  const TargetSubtargetInfo& TSI = I.getMF()->getSubtarget();
  const TargetInstrInfo& TII = *TSI.getInstrInfo();
  const TargetRegisterInfo& TRI = *TSI.getRegisterInfo();
  const RegisterBankInfo& RBI = *TSI.getRegBankInfo();
  const MachineRegisterInfo& MRI = I.getMF()->getRegInfo();
  const LLT s8 = LLT::scalar(8);

  switch (I.getOpcode()) {
  case MOS6502::COPY: {
    Register Dst = I.getOperand(0).getReg();
    if (!MOS6502::GPRRegClass.contains(Dst)) break;
    Optional<int64_t> Cst = getConstantVRegVal(I.getOperand(1).getReg(), MRI);
    if (!Cst) break;
    MachineIRBuilder Builder(I);
    Builder.buildInstr(MOS6502::LDimm).addDef(Dst).addImm(*Cst);
    I.removeFromParent();
    break;
  }
  }

  switch (I.getOpcode()) {
  case MOS6502::COPY:
    return true;
  }
  return false;
}

void MOS6502InstructionSelector::setupGeneratedPerFunctionState(MachineFunction &MF) {
  // Disable assertion in base class; we're not using SelectionDAG TableGen.
}

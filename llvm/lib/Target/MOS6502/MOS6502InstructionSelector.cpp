#include "MOS6502InstructionSelector.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"

#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/GlobalISel/RegisterBankInfo.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Constants.h"

using namespace llvm;

bool MOS6502InstructionSelector::select(MachineInstr &I) {
  const TargetSubtargetInfo& TSI = I.getMF()->getSubtarget();
  const TargetInstrInfo& TII = *TSI.getInstrInfo();
  const TargetRegisterInfo& TRI = *TSI.getRegisterInfo();
  const RegisterBankInfo& RBI = *TSI.getRegBankInfo();
  const MachineRegisterInfo& MRI = I.getMF()->getRegInfo();
  const LLT s8 = LLT::scalar(8);

  if (I.getOpcode() == MOS6502::G_CONSTANT) {
    assert(MRI.getType(I.getOperand(0).getReg()) == s8);
    assert(I.getOperand(1).getCImm()->getBitWidth() == 8);
    I.setDesc(TII.get(MOS6502::CONSTANT));
    return constrainSelectedInstRegOperands(I, TII, TRI, RBI);
  }
  return false;
}

void MOS6502InstructionSelector::setupGeneratedPerFunctionState(MachineFunction &MF) {
  // Disable assertion in base class; we're not using SelectionDAG TableGen.
}

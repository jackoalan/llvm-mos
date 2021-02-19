#include "MOSRegisterBankInfo.h"
#include "MCTargetDesc/MOSMCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/RegisterBank.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

#define GET_TARGET_REGBANK_IMPL
#include "MOSGenRegisterBank.inc"

using namespace llvm;

const RegisterBankInfo::InstructionMapping &
MOSRegisterBankInfo::getInstrMapping(const MachineInstr &MI) const {
  const auto &Mapping = getInstrMappingImpl(MI);
  if (Mapping.isValid()) return Mapping;

  const auto &MRI = MI.getMF()->getRegInfo();
  unsigned NumOperands = MI.getNumOperands();

  SmallVector<const ValueMapping*, 8> ValMappings(NumOperands);
  for (unsigned Idx = 0; Idx < NumOperands; ++Idx) {
    const auto &Operand = MI.getOperand(Idx);
    if (!Operand.isReg()) continue;
    LLT Ty = MRI.getType(Operand.getReg());
    ValMappings[Idx] =
      &getValueMapping(0, Ty.getSizeInBits(), MOS::AnyRegBank);
  }
  return getInstructionMapping(/*ID=*/1, /*Cost=*/1,
                               getOperandsMapping(ValMappings), NumOperands);
}

void MOSRegisterBankInfo::applyMappingImpl(const OperandsMapper &OpdMapper)
  const {
  applyDefaultMapping(OpdMapper);
}

const RegisterBank &
MOSRegisterBankInfo::getRegBankFromRegClass(const TargetRegisterClass &RC,
                                                LLT Ty) const {
  return MOS::AnyRegBank;
}

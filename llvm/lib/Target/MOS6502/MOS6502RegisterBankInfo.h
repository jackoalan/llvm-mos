
#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502REGISTERBANKINFO_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502REGISTERBANKINFO_H

#include "llvm/CodeGen/GlobalISel/RegisterBankInfo.h"

#define GET_REGBANK_DECLARATIONS
#include "MOS6502GenRegisterBank.inc"

namespace llvm {

class MOS6502GenRegisterBankInfo : public RegisterBankInfo {
protected:
#define GET_TARGET_REGBANK_CLASS
#include "MOS6502GenRegisterBank.inc"
};

/// This class provides the information for the target register banks.
class MOS6502RegisterBankInfo final : public MOS6502GenRegisterBankInfo {
 public:
  const InstructionMapping &
  getInstrMapping(const MachineInstr &MI) const override;

  void applyMappingImpl(const OperandsMapper &OpdMapper) const override;

  const RegisterBank &
  getRegBankFromRegClass(const TargetRegisterClass &RC, LLT Ty) const override;
};

}  // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502REGISTERBANKINFO_H

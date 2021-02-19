
#ifndef LLVM_LIB_TARGET_MOS_MOSREGISTERBANKINFO_H
#define LLVM_LIB_TARGET_MOS_MOSREGISTERBANKINFO_H

#include "llvm/CodeGen/GlobalISel/RegisterBankInfo.h"

#define GET_REGBANK_DECLARATIONS
#include "MOSGenRegisterBank.inc"

namespace llvm {

class MOSGenRegisterBankInfo : public RegisterBankInfo {
protected:
#define GET_TARGET_REGBANK_CLASS
#include "MOSGenRegisterBank.inc"
};

/// This class provides the information for the target register banks.
class MOSRegisterBankInfo final : public MOSGenRegisterBankInfo {
 public:
  const InstructionMapping &
  getInstrMapping(const MachineInstr &MI) const override;

  void applyMappingImpl(const OperandsMapper &OpdMapper) const override;

  const RegisterBank &
  getRegBankFromRegClass(const TargetRegisterClass &RC, LLT Ty) const override;
};

}  // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS_MOSREGISTERBANKINFO_H

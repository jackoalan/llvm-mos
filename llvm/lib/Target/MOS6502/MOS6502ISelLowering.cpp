#include "MOS6502ISelLowering.h"

#include "MOS6502RegisterInfo.h"
#include "MOS6502Subtarget.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "llvm/CodeGen/TargetLowering.h"

using namespace llvm;

MOS6502TargetLowering::MOS6502TargetLowering(const TargetMachine &TM,
                                             const MOS6502Subtarget &STI)
    : TargetLowering(TM) {
  addRegisterClass(MVT::i8, &MOS6502::GPRRegClass);
  computeRegisterProperties(STI.getRegisterInfo());
}

MVT MOS6502TargetLowering::getRegisterTypeForCallingConv(LLVMContext &Context,
                                                         CallingConv::ID CC,
                                                         EVT VT) const {
  if (VT == MVT::i16) {
    return MVT::i8;
  }
  return TargetLowering::getRegisterTypeForCallingConv(Context, CC, VT);
}

unsigned MOS6502TargetLowering::getNumRegistersForCallingConv(
    LLVMContext &Context, CallingConv::ID CC, EVT VT) const {
  if (VT == MVT::i16) {
    return 2;
  }
  return TargetLowering::getNumRegistersForCallingConv(Context, CC, VT);
}

TargetLowering::ConstraintType
MOS6502TargetLowering::getConstraintType(StringRef Constraint) const {
  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
    default:
      break;
    case 'a':
    case 'x':
    case 'y':
      return C_Register;
    }
  }
  return TargetLowering::getConstraintType(Constraint);
}

std::pair<unsigned, const TargetRegisterClass *>
MOS6502TargetLowering::getRegForInlineAsmConstraint(
    const TargetRegisterInfo *TRI, StringRef Constraint, MVT VT) const {
  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
    default:
      break;
    case 'r':
      return std::make_pair(0U, &MOS6502::GPRRegClass);
    case 'a':
      return std::make_pair(MOS6502::A, &MOS6502::GPRRegClass);
    case 'x':
      return std::make_pair(MOS6502::X, &MOS6502::GPRRegClass);
    case 'y':
      return std::make_pair(MOS6502::Y, &MOS6502::GPRRegClass);
    }
  }

  return TargetLowering::getRegForInlineAsmConstraint(TRI, Constraint, VT);
}

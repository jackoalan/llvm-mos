#include "MOS6502ISelLowering.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502RegisterInfo.h"
#include "MOS6502Subtarget.h"
#include "llvm/CodeGen/TargetLowering.h"

using namespace llvm;

MOS6502TargetLowering::MOS6502TargetLowering(const TargetMachine &TM,
                                             const MOS6502Subtarget &STI)
    : TargetLowering(TM) {
  addRegisterClass(MVT::i8, &MOS6502::GPRRegClass);
  addRegisterClass(MVT::i16, &MOS6502::ZP_PTRRegClass);
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

bool MOS6502TargetLowering::isLegalAddressingMode(const DataLayout &DL,
                                                  const AddrMode &AM, Type *Ty,
                                                  unsigned AddrSpace,
                                                  Instruction *I) const {
  // In general, the basereg and scalereg are the 16-bit GEP index type, which
  // cannot be natively supported.

  if (AM.Scale) return false;

  if (AM.HasBaseReg) {
    // A 16-bit base reg can be placed into a ZP_PTR register, then the base
    // offset added using the Y indexed addressing mode. This requires the Y
    // index reg as well as the base reg, but that's what it's there for.
    return !AM.BaseGV && 0 <= AM.BaseOffs && AM.BaseOffs < 256;
  }

  // Any other combination of GV and BaseOffset are just global offsets.
  return true;
}

bool MOS6502TargetLowering::shouldLocalize(
    const MachineInstr &MI, const TargetTransformInfo *TTI) const {
  // Only frame indices are tricky to rematerialize; all other constants are
  // legalized, however indirectly, to separable 8-bit immediate operands.
  return MI.getOpcode() == TargetOpcode::G_FRAME_INDEX;
}

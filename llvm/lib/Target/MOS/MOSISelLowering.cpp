//===-- MOSISelLowering.cpp - MOS DAG Lowering Implementation -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that MOS uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#include "MOSISelLowering.h"

#include "MCTargetDesc/MOSMCTargetDesc.h"
#include "MOSRegisterInfo.h"
#include "MOSSubtarget.h"
#include "llvm/CodeGen/TargetLowering.h"

using namespace llvm;

MOSTargetLowering::MOSTargetLowering(const TargetMachine &TM,
                                             const MOSSubtarget &STI)
    : TargetLowering(TM) {
  addRegisterClass(MVT::i8, &MOS::GPRRegClass);
  addRegisterClass(MVT::i16, &MOS::ZP_PTRRegClass);
  computeRegisterProperties(STI.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(MOS::SP);
}

MVT MOSTargetLowering::getRegisterTypeForCallingConv(LLVMContext &Context,
                                                         CallingConv::ID CC,
                                                         EVT VT) const {
  if (VT == MVT::i16) {
    return MVT::i8;
  }
  return TargetLowering::getRegisterTypeForCallingConv(Context, CC, VT);
}

unsigned MOSTargetLowering::getNumRegistersForCallingConv(
    LLVMContext &Context, CallingConv::ID CC, EVT VT) const {
  if (VT == MVT::i16) {
    return 2;
  }
  return TargetLowering::getNumRegistersForCallingConv(Context, CC, VT);
}

TargetLowering::ConstraintType
MOSTargetLowering::getConstraintType(StringRef Constraint) const {
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
MOSTargetLowering::getRegForInlineAsmConstraint(
    const TargetRegisterInfo *TRI, StringRef Constraint, MVT VT) const {
  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
    default:
      break;
    case 'r':
      return std::make_pair(0U, &MOS::GPRRegClass);
    case 'a':
      return std::make_pair(MOS::A, &MOS::GPRRegClass);
    case 'x':
      return std::make_pair(MOS::X, &MOS::GPRRegClass);
    case 'y':
      return std::make_pair(MOS::Y, &MOS::GPRRegClass);
    }
  }

  return TargetLowering::getRegForInlineAsmConstraint(TRI, Constraint, VT);
}

bool MOSTargetLowering::isLegalAddressingMode(const DataLayout &DL,
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

bool MOSTargetLowering::shouldLocalize(
    const MachineInstr &MI, const TargetTransformInfo *TTI) const {
  // Only frame indices are tricky to rematerialize; all other constants are
  // legalized, however indirectly, to separable 8-bit immediate operands.
  return MI.getOpcode() == TargetOpcode::G_FRAME_INDEX;
}

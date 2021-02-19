//===-- MOSISelLowering.h - MOS DAG Lowering Interface ----------*- C++ -*-===//
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

#ifndef LLVM_LIB_TARGET_MOS_MOSISELLOWERING_H
#define LLVM_LIB_TARGET_MOS_MOSISELLOWERING_H

#include "llvm/CodeGen/TargetLowering.h"

#include "llvm/Target/TargetMachine.h"

namespace llvm {

class MOSSubtarget;

class MOSTargetLowering : public TargetLowering {
public:
  MOSTargetLowering(const TargetMachine &TM, const MOSSubtarget &STI);

  MVT getRegisterTypeForCallingConv(LLVMContext &Context, CallingConv::ID CC,
                                    EVT VT) const override;

  unsigned getNumRegistersForCallingConv(LLVMContext &Context,
                                         CallingConv::ID CC,
                                         EVT VT) const override;

  ConstraintType getConstraintType(StringRef Constraint) const override;

  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                               StringRef Constraint, MVT VT) const override;

  bool isLegalAddressingMode(const DataLayout &DL, const AddrMode &AM, Type *Ty,
                             unsigned AddrSpace,
                             Instruction *I = nullptr) const override;

  bool shouldLocalize(const MachineInstr &MI,
                      const TargetTransformInfo *TTI) const override;
};

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS_MOSISELLOWERING_H
public:
  explicit MOSTargetLowering(const MOSTargetMachine &TM,
                             const MOSSubtarget &STI);
protected:
  const MOSSubtarget &Subtarget;
};

} // end namespace llvm

#endif // LLVM_MOS_ISEL_LOWERING_H

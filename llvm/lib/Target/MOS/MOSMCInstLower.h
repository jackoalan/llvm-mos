//===-- MOSMCInstLower.h - Lower MachineInstr to MCInst ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIB_TARGET_MOS_MOSMCINSTLOWER_H
#define LLVM_LIB_TARGET_MOS_MOSMCINSTLOWER_H

#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCContext.h"

namespace llvm {

class MOSMCInstLower {
  MCContext &Ctx;
  const AsmPrinter &AP;

public:
  MOSMCInstLower(MCContext &Ctx, const AsmPrinter &AP) : Ctx(Ctx), AP(AP) {}

  void lower(const MachineInstr *MI, MCInst &OutMI);

private:
  bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp);
  const MCExpr *applyTargetFlags(unsigned Flags, const MCExpr *Expr);
};

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS_MOSMCINSTLOWER_H


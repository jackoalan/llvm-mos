#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502MCINSTLOWER_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502MCINSTLOWER_H

#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCContext.h"

namespace llvm {

class MOS6502MCInstLower {
  MCContext &Ctx;
  const AsmPrinter &AP;

public:
  MOS6502MCInstLower(MCContext &Ctx, const AsmPrinter &AP) : Ctx(Ctx), AP(AP) {}

  void lower(const MachineInstr *MI, MCInst &OutMI);

private:
  bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp);
  const MCExpr *applyTargetFlags(unsigned Flags, const MCExpr *Expr);
};

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502MCINSTLOWER_H

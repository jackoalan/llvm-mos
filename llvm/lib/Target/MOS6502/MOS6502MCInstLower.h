#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502MCINSTLOWER_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502MCINSTLOWER_H

#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCContext.h"

namespace llvm {

void LowerMOS6502MachineInstrToMCInst(const MachineInstr *MI, MCInst &OutMI,
                                      const AsmPrinter &AP, MCContext &Ctx);

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502MCINSTLOWER_H

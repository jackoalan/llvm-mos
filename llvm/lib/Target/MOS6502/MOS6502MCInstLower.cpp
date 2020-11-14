#include "MOS6502MCInstLower.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

void llvm::LowerMOS6502MachineInstrToMCInst(const MachineInstr *MI,
                                            MCInst &OutMI,
                                            const AsmPrinter &AP) {
  report_fatal_error("Not yet implemented.");
}

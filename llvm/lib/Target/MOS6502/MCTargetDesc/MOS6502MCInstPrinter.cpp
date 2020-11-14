#include "MOS6502MCInstPrinter.h"

#include "llvm/MC/MCInst.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "MOS6502GenAsmWriter.inc"

void MOS6502InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                   StringRef Annot, const MCSubtargetInfo &STI,
                                   raw_ostream &O) {
  report_fatal_error("Not yet implemented.");
}

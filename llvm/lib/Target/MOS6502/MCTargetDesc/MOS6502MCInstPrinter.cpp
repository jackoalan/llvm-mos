#include "MOS6502MCInstPrinter.h"

using namespace llvm;


void MOS6502InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                   StringRef Annot, const MCSubtargetInfo &STI,
                                   raw_ostream &O) {
  report_fatal_error("Not yet implemented.");
}

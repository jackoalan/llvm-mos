
#ifndef LLVM_LIB_TARGET_MOS6502_MCTARGETDESC_MOS6502INSTPRINTER_H
#define LLVM_LIB_TARGET_MOS6502_MCTARGETDESC_MOS6502INSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"

namespace llvm {

class MOS6502InstPrinter : public MCInstPrinter {
public:
  MOS6502InstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                   const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}

  void printInst(const MCInst *MI, uint64_t Address, StringRef Annot,
                 const MCSubtargetInfo &STI, raw_ostream &O) override;
};

} // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MCTARGETDESC_MOS6502INSTPRINTER_H

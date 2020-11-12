#ifndef LLVM_MC_MCSECTIONMOS6502_H
#define LLVM_MC_MCSECTIONMOS6502_H

#include "llvm/MC/MCSection.h"

namespace llvm {

class MCSectionMOS6502 : public MCSection {
public:
  MCSectionMOS6502(StringRef Name, SectionKind Kind, MCSymbol* Begin);

  void PrintSwitchToSection(const MCAsmInfo &MAI, const Triple &T,
                            raw_ostream &OS,
                            const MCExpr *Subsection) const override;

  bool UseCodeAlign() const override { return false; }
  bool isVirtualSection() const override { return false; }
};

} //  namespace llvm

#endif  // not LLVM_MC_MCSECTIONMOS6502_H

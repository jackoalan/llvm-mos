#include "llvm/MC/MCSectionMOS6502.h"

using namespace llvm;

MCSectionMOS6502::MCSectionMOS6502(StringRef Name, SectionKind Kind, MCSymbol* Begin)
  : MCSection(SV_MOS6502, Name, Kind, Begin) {};

void MCSectionMOS6502::PrintSwitchToSection(const MCAsmInfo &MAI, const Triple &T,
                                            raw_ostream &OS,
                                            const MCExpr *Subsection) const {
  report_fatal_error("Not yet implemented.");
}

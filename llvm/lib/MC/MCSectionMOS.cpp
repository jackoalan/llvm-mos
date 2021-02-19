#include "llvm/MC/MCSectionMOS.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

MCSectionMOS::MCSectionMOS(StringRef Name, SectionKind Kind,
                                   MCSymbol *Begin)
    : MCSection(SV_MOS, Name, Kind, Begin) {}

void MCSectionMOS::PrintSwitchToSection(const MCAsmInfo &MAI,
                                            const Triple &T, raw_ostream &OS,
                                            const MCExpr *Subsection) const {
  OS << StringSwitch<std::string>(Name)
            .Case("BSS", ".bss")
            .Case("CODE", ".code")
            .Case("DATA", ".data")
            .Case("RODATA", ".rodata")
            .Case("ZEROPAGE", ".zeropage")
    .Default((".segment\t\"" + getName() + "\"").str())
     << "\n";
}

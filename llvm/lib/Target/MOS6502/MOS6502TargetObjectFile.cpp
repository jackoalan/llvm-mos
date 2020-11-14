#include "MOS6502TargetObjectFile.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionMOS6502.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

MCSection *MOS6502TargetObjectFile::SelectSectionForGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {
  if (TM.getFunctionSections() || TM.getDataSections()) {
    report_fatal_error("Unique sections not supported on MOS6502.");
  }
  StringRef Name;
  if (Kind.isText()) {
    Name = ".text";
  } else {
    report_fatal_error("Section kind not supported.");
  }
  return getContext().getMOS6502Section(Name, Kind);
}

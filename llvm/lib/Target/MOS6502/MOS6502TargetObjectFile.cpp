#include "MOS6502TargetObjectFile.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionMOS6502.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

MCSection *MOS6502TargetObjectFile::SelectSectionForGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {
  if (TM.getFunctionSections() || TM.getDataSections()) {
    report_fatal_error("Unique sections not supported on MOS6502.");
  }
  if (GO->getAddressSpace() == 1) {
    return getZPSection();
  } else if (Kind.isText()) {
    return getTextSection();
  } else if (Kind.isData()) {
    return getDataSection();
  } else if (Kind.isReadOnly()) {
    return getReadOnlySection();
  } else if (Kind.isBSS()) {
    return getBSSSection();
  }
  report_fatal_error("Section kind not supported.");
}

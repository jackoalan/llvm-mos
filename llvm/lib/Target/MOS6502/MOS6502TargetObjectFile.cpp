#include "MOS6502TargetObjectFile.h"
#include "llvm/ADT/StringExtras.h"
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
  if (Kind.isText()) {
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

void MOS6502TargetObjectFile::getNameWithPrefix(SmallVectorImpl<char> &OutName,
                                                const GlobalValue *GV,
                                                const TargetMachine &TM) const {
  SmallVector<char, 8> UnescapedName;
  TargetLoweringObjectFile::getNameWithPrefix(UnescapedName, GV, TM);
  raw_svector_ostream OS(OutName);
  for (char c : UnescapedName) {
    if (isAlnum(c)) {
      OS << c;
    } else if (c == '_') {
      OS << "__";
    } else {
      OS << '_' << utohexstr(c);
    }
  }
}

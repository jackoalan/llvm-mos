#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502TARGETOBJECTFILE_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502TARGETOBJECTFILE_H

#include "llvm/MC/SectionKind.h"
#include "llvm/Target/TargetLoweringObjectFile.h"

namespace llvm {

class MOS6502TargetObjectFile : public TargetLoweringObjectFile {
  MCSection *getExplicitSectionGlobal(const GlobalObject *GO, SectionKind Kind,
                                      const TargetMachine &TM) const override {
    report_fatal_error("Not yet implemented.");
  }

  MCSection *SelectSectionForGlobal(const GlobalObject *GO, SectionKind Kind,
                                    const TargetMachine &TM) const override;
};

} // namespace llvm

#endif

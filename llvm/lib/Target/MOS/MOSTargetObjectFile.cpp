//===-- MOSTargetObjectFile.cpp - MOS Object Files ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MOSTargetObjectFile.h"

MCSection *MOSTargetObjectFile::SelectSectionForGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {
  if (TM.getFunctionSections() || TM.getDataSections()) {
    report_fatal_error("Unique sections not supported on MOS.");
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
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {
  // Global values in flash memory are placed in the progmem.data section
  // unless they already have a user assigned section.
  if (MOS::isProgramMemoryAddress(GO) && !GO->hasSection())
    return ProgmemDataSection;

  // Otherwise, we work the same way as ELF.
  return Base::SelectSectionForGlobal(GO, Kind, TM);
}
} // end of namespace llvm

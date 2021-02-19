//===-- MOSMCAsmInfo.cpp - MOS asm properties -----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the MOSMCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "MOSMCAsmInfo.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/MC/MCDirectives.h"

using namespace llvm;

MOSMCAsmInfo::MOSMCAsmInfo() {
  AsciiDirective = nullptr;
  AscizDirective = nullptr;
  ByteListDirective = "\t.byt\t";
  CalleeSaveStackSlotSize = 1;
  CharacterLiteralSyntax = ACLS_Decimal;
  CodePointerSize = 2;
  CommentString = ";";
  Data16bitsDirective = "\t.word\t";
  Data32bitsDirective = nullptr;
  Data64bitsDirective = nullptr;
  Data8bitsDirective = "\t.byt\t";
  GlobalDirective = ".global\t";
  HasCommonSymbolDirective = false;
  HasDotTypeDotSizeDirective = false;
  HasSingleParameterDotFile = false;
  HiddenDeclarationVisibilityAttr = MCSA_Invalid;
  HiddenVisibilityAttr = MCSA_Invalid;
  MaxInstLength = 3;
  ProtectedVisibilityAttr = MCSA_Invalid;
  SeparatorString = nullptr;
  UseEqualsForAssignment = true;
  UseIntegratedAssembler = false;
  WeakDirective = nullptr;
  ZeroDirective = "\t.res\t";
}

bool MOSMCAsmInfo::isAcceptableChar(char C) const {
  // Note: _ is not an acceptable character, since it must also be escaped to
  // preserve unique decodablility.
  return (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') ||
         (C >= '0' && C <= '9');
}

void MOSMCAsmInfo::printEscapedName(raw_ostream &OS, StringRef Name) const {
  for (char C : Name) {
    if (C == '_') {
      OS << "__";
    } else if (isAcceptableChar(C)) {
      OS << C;
    } else {
      OS << '_' << utohexstr(C);
    }
  }
}

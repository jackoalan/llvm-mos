#include "MOS6502MCAsmInfo.h"
#include "llvm/MC/MCDirectives.h"

using namespace llvm;

MOS6502MCAsmInfo::MOS6502MCAsmInfo() {
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
  HasDotTypeDotSizeDirective = false;
  HasSingleParameterDotFile = false;
  HiddenDeclarationVisibilityAttr = MCSA_Invalid;
  HiddenVisibilityAttr = MCSA_Invalid;
  MaxInstLength = 3;
  ProtectedVisibilityAttr = MCSA_Invalid;
  SeparatorString = nullptr;
  UseIntegratedAssembler = false;
  WeakDirective = nullptr;
  ZeroDirective = "\t.res\t";
}

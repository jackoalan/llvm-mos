#include "MOS6502MCAsmInfo.h"
#include "llvm/MC/MCDirectives.h"

using namespace llvm;

MOS6502MCAsmInfo::MOS6502MCAsmInfo() {
  CodePointerSize = 2;
  CalleeSaveStackSlotSize = 1;
  MaxInstLength = 3;
  SeparatorString = nullptr;
  CommentString = ";";
  ZeroDirective = "\t.res\t";
  AsciiDirective = nullptr;
  AscizDirective = nullptr;
  ByteListDirective = "\t.byt\t";
  Data8bitsDirective = "\t.byt\t";
  Data16bitsDirective = "\t.word\t";
  Data32bitsDirective = nullptr;
  Data64bitsDirective = nullptr;
  GlobalDirective = ".global\t";
  HasDotTypeDotSizeDirective = false;
  HasSingleParameterDotFile = false;
  WeakDirective = nullptr;
  HiddenVisibilityAttr = MCSA_Invalid;
  HiddenDeclarationVisibilityAttr = MCSA_Invalid;
  ProtectedVisibilityAttr = MCSA_Invalid;
  UseIntegratedAssembler = false;
}

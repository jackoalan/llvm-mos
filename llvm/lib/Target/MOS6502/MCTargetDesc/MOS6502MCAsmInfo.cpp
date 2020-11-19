#include "MOS6502MCAsmInfo.h"
#include "llvm/MC/MCDirectives.h"

using namespace llvm;

MOS6502MCAsmInfo::MOS6502MCAsmInfo() {
  CodePointerSize = 2;
  CalleeSaveStackSlotSize = 1;
  MaxInstLength = 3;
  SeparatorString = nullptr;
  CommentString = ";";
  ZeroDirective = ".res";
  AsciiDirective = nullptr;
  AscizDirective = nullptr;
  ByteListDirective = ".byt";
  Data8bitsDirective = ".byt";
  Data16bitsDirective = ".word";
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

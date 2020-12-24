#include "MOS6502.h"

using namespace clang::targets;

MOS6502TargetInfo::MOS6502TargetInfo(const llvm::Triple &Triple,
                                     const TargetOptions &)
    : TargetInfo(Triple) {
  static const char Layout[] =
      "e-p:16:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-a:8:8-Fi8-n8";
  resetDataLayout(Layout);

  PointerWidth = 16;
  PointerAlign = 8;
  IntWidth = 16;
  IntAlign = 8;
  LongAlign = 8;
  LongLongAlign = 8;
  ShortAccumAlign = 8;
  AccumWidth = 16;
  AccumAlign = 8;
  LongAccumAlign = 8;
  FractWidth = FractAlign = 8;
  LongFractAlign = 8;
  AccumScale = 7;
  SuitableAlign = 8;
  DefaultAlignForAttributeAligned = 8;
  SizeType = UnsignedShort;
  PtrDiffType = SignedShort;
  IntPtrType = SignedShort;
  WCharType = UnsignedLong;
  WIntType = UnsignedLong;
  Char32Type = UnsignedLong;
  SigAtomicType = UnsignedChar;
}

bool MOS6502TargetInfo::validateAsmConstraint(
    const char *&Name, TargetInfo::ConstraintInfo &Info) const {
  switch (*Name) {
  default:
    return false;
  case 'a':
  case 'x':
  case 'y':
    // The A, X, or Y register
    Info.setAllowsRegister();
    return true;
  }
  return false;
}

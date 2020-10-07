
#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_MOS6502_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_MOS6502_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/Compiler.h"

namespace clang {
namespace targets {

class MOS6502TargetInfo : public TargetInfo {
private:
 public:
  MOS6502TargetInfo(const llvm::Triple &Triple, const TargetOptions &) : TargetInfo(Triple) {
    static const char Layout[] =
      "e-p:16:8:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-a:8:8-Fi8-n8";
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
    IntPtrType = SignedLong;
    WCharType = SignedLong;
    WIntType = SignedLong;
    Char32Type = UnsignedLong;
    SigAtomicType = SignedChar;
  }

  void getTargetDefines(const LangOptions &Opts, MacroBuilder &Builder) const override {}

  ArrayRef<Builtin::Info> getTargetBuiltins() const override { return None; }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    return false;
  }

  const char *getClobbers() const override { return ""; }

  ArrayRef<const char *> getGCCRegNames() const override { return None; }
  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override { return None; }
  unsigned getRegisterWidth() const override { return 8; }
};

} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_MOS6502_H

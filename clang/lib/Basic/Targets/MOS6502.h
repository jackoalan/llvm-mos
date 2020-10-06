
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
    resetDataLayout("e-p:16:8:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-a:8:8-Fi8-n8");
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
};

} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_MOS6502_H

#include "MOS6502TargetInfo.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheMOS6502Target() {
  static Target TheMOS6502Target;
  return TheMOS6502Target;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMOS6502TargetInfo() {
  RegisterTarget<Triple::mos6502> X(getTheMOS6502Target(), "mos6502",
                                    "MOS 6502", "MOS6502");
}

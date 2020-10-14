#include "MOS6502MCAsmInfo.h"
#include "TargetInfo/MOS6502TargetInfo.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

static MCAsmInfo *createMOS6502MCAsmInfo(const MCRegisterInfo &MRI,
                                         const Triple &TT,
                                         const MCTargetOptions &Options) {
  return new MOS6502MCAsmInfo;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMOS6502TargetMC() {
  TargetRegistry::RegisterMCAsmInfo(getTheMOS6502Target(), createMOS6502MCAsmInfo);
}

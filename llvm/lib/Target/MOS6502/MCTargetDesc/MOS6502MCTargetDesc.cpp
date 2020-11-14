#include "MOS6502MCTargetDesc.h"

#include "MOS6502InstPrinter.h"
#include "MOS6502MCAsmInfo.h"
#include "TargetInfo/MOS6502TargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "MOS6502GenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "MOS6502GenRegisterInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "MOS6502GenSubtargetInfo.inc"

using namespace llvm;

static MCAsmInfo *createMOS6502MCAsmInfo(const MCRegisterInfo &MRI,
                                         const Triple &TT,
                                         const MCTargetOptions &Options) {
  return new MOS6502MCAsmInfo;
}

static MCInstPrinter *createMOS6502InstPrinter(const Triple &T,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  return new MOS6502InstPrinter(MAI, MII, MRI);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMOS6502TargetMC() {
  Target &T = getTheMOS6502Target();
  TargetRegistry::RegisterMCAsmInfo(T, createMOS6502MCAsmInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createMOS6502InstPrinter);
}

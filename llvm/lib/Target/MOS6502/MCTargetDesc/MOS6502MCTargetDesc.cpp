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

static MCRegisterInfo *createMOS6502MCRegisterInfo(const Triple &Triple) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitMOS6502MCRegisterInfo(X, 0);
  return X;
}

static MCInstrInfo *createMOS6502MCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitMOS6502MCInstrInfo(X);
  return X;
}

static MCInstPrinter *createMOS6502InstPrinter(const Triple &T,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  return new MOS6502InstPrinter(MAI, MII, MRI);
}

static MCSubtargetInfo *
createMOS6502MCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  return createMOS6502MCSubtargetInfoImpl(TT, CPU, /*TuneCPU*/ CPU, FS);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMOS6502TargetMC() {
  Target &T = getTheMOS6502Target();
  TargetRegistry::RegisterMCAsmInfo(T, createMOS6502MCAsmInfo);
  TargetRegistry::RegisterMCRegInfo(T, createMOS6502MCRegisterInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createMOS6502MCInstrInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createMOS6502InstPrinter);
  TargetRegistry::RegisterMCSubtargetInfo(T, createMOS6502MCSubtargetInfo);
}

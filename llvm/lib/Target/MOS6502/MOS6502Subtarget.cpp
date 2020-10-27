#include "MOS6502Subtarget.h"

#include "MOS6502CallLowering.h"
#include "MOS6502InstructionSelector.h"
#include "MOS6502LegalizerInfo.h"
#include "MOS6502RegisterBankInfo.h"

using namespace llvm;

#define DEBUG_TYPE "mos6502-subtarget"

#define GET_SUBTARGETINFO_CTOR
#include "MOS6502GenSubtargetInfo.inc"

MOS6502Subtarget::MOS6502Subtarget(const Triple &TT, StringRef CPU,
                                   StringRef FS, const TargetMachine &TM)
  : MOS6502GenSubtargetInfo(TT, CPU, /*TuneCPU=*/CPU, FS), TLInfo(TM) {
  CallLoweringInfo.reset(new MOS6502CallLowering(getTargetLowering()));
  Legalizer.reset(new MOS6502LegalizerInfo);
  RegBankInfo.reset(new MOS6502RegisterBankInfo);
  InstSelector.reset(new MOS6502InstructionSelector);
}

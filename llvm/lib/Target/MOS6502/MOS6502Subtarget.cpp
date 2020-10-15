#include "MOS6502Subtarget.h"

using namespace llvm;

#define DEBUG_TYPE "mos6502-subtarget"

#define GET_SUBTARGETINFO_CTOR
#include "MOS6502GenSubtargetInfo.inc"

MOS6502Subtarget::MOS6502Subtarget(const Triple &TT, StringRef CPU,
                                   StringRef TuneCPU, StringRef FS)
  : MOS6502GenSubtargetInfo(TT, CPU, TuneCPU, FS) {}

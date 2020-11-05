#include "MOS6502ISelLowering.h"

#include "MOS6502Subtarget.h"

using namespace llvm;

MOS6502TargetLowering::MOS6502TargetLowering(const TargetMachine &TM,
                                             const MOS6502Subtarget& STI)
  : TargetLowering(TM) {
  computeRegisterProperties(STI.getRegisterInfo());
}

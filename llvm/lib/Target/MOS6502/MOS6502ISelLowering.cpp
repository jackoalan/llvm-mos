#include "MOS6502ISelLowering.h"

#include "MOS6502RegisterInfo.h"
#include "MOS6502Subtarget.h"

using namespace llvm;

MOS6502TargetLowering::MOS6502TargetLowering(const TargetMachine &TM,
                                             const MOS6502Subtarget& STI)
  : TargetLowering(TM) {
  addRegisterClass(MVT::i8, &MOS6502::I8RegClass);
  addRegisterClass(MVT::i16, &MOS6502::I16RegClass);
  computeRegisterProperties(STI.getRegisterInfo());
}

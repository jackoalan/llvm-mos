#include "MOS6502ISelLowering.h"

#include "MOS6502RegisterInfo.h"
#include "MOS6502Subtarget.h"

using namespace llvm;

MOS6502TargetLowering::MOS6502TargetLowering(const TargetMachine &TM,
                                             const MOS6502Subtarget& STI)
  : TargetLowering(TM) {
  addRegisterClass(MVT::i8, &MOS6502::GPRRegClass);
  computeRegisterProperties(STI.getRegisterInfo());
}

MVT MOS6502TargetLowering::getRegisterTypeForCallingConv(LLVMContext &Context,
                                                         CallingConv::ID CC,
                                                         EVT VT) const {
  if (VT == MVT::i16) {
    return MVT::i8;
  }
  return TargetLowering::getRegisterTypeForCallingConv(Context, CC, VT);
}

unsigned MOS6502TargetLowering::getNumRegistersForCallingConv(LLVMContext &Context,
                                                              CallingConv::ID CC,
                                                              EVT VT) const {
  if (VT == MVT::i16) {
    return 2;
  }
  return TargetLowering::getNumRegistersForCallingConv(Context, CC, VT);
}

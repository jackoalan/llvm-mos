#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502SUBTARGET_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502SUBTARGET_H

#include "MOS6502FrameLowering.h"
#include "MOS6502ISelLowering.h"
#include "MOS6502InstrInfo.h"
#include "MOS6502RegisterInfo.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"

#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "MOS6502GenSubtargetInfo.inc"

namespace llvm {

class MOS6502Subtarget : public MOS6502GenSubtargetInfo {
  MOS6502FrameLowering FrameLowering;
  MOS6502InstrInfo InstrInfo;
  MOS6502RegisterInfo RegInfo;
  MOS6502TargetLowering TLInfo;

  std::unique_ptr<CallLowering> CallLoweringInfo;
  std::unique_ptr<LegalizerInfo> Legalizer;
  std::unique_ptr<RegisterBankInfo> RegBankInfo;
  std::unique_ptr<InstructionSelector> InstSelector;

public:
  MOS6502Subtarget(const Triple &TT, StringRef CPU, StringRef FS,
                   const TargetMachine& TM);

  const MOS6502FrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const MOS6502InstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const MOS6502RegisterInfo *getRegisterInfo() const override {
    return &RegInfo;
  }
  const MOS6502TargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }

  const CallLowering *getCallLowering() const override {
    return CallLoweringInfo.get();
  }
  const LegalizerInfo *getLegalizerInfo() const override {
    return Legalizer.get();
  }
  const RegisterBankInfo *getRegBankInfo() const override {
    return RegBankInfo.get();
  }
  InstructionSelector *getInstructionSelector() const override {
    return InstSelector.get();
  }
};

}  // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502SUBTARGET_H

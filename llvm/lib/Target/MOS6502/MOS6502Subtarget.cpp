#include "MOS6502Subtarget.h"

#include "MOS6502CallLowering.h"
#include "MOS6502ISelLowering.h"
#include "MOS6502InstructionSelector.h"
#include "MOS6502LegalizerInfo.h"
#include "MOS6502RegisterBankInfo.h"
#include "llvm/CodeGen/GlobalISel/InlineAsmLowering.h"
#include "llvm/CodeGen/MachineScheduler.h"

using namespace llvm;

#define DEBUG_TYPE "mos6502-subtarget"

#define GET_SUBTARGETINFO_CTOR
#include "MOS6502GenSubtargetInfo.inc"

MOS6502Subtarget::MOS6502Subtarget(const Triple &TT, StringRef CPU,
                                   StringRef FS, const TargetMachine &TM)
    : MOS6502GenSubtargetInfo(TT, CPU, /*TuneCPU=*/CPU, FS),
      TLInfo(TM, initializeSubtargetDependencies(TT, CPU, CPU, FS)) {
  CallLoweringInfo.reset(new MOS6502CallLowering(getTargetLowering()));
  Legalizer.reset(new MOS6502LegalizerInfo);

  auto *RBI = new MOS6502RegisterBankInfo;
  RegBankInfo.reset(RBI);
  InstSelector.reset(createMOS6502InstructionSelector(
      *static_cast<const MOS6502TargetMachine *>(&TM), *this, *RBI));
  InlineAsmLoweringInfo.reset(new InlineAsmLowering(getTargetLowering()));
}

MOS6502Subtarget &MOS6502Subtarget::initializeSubtargetDependencies(
    const Triple &TT, StringRef CPU, StringRef TuneCPU, StringRef FS) {
  std::string CPUName = std::string(CPU);
  std::string TuneCPUName = std::string(TuneCPU);
  if (CPUName.empty())
    CPUName = "generic";
  if (TuneCPUName.empty())
    TuneCPUName = CPUName;
  InitMCProcessorInfo(CPUName, TuneCPUName, FS);

  return *this;
}

void MOS6502Subtarget::overrideSchedPolicy(MachineSchedPolicy &Policy,
                                           unsigned NumRegionInstrs) const {
  Policy.OnlyBottomUp = false;
  Policy.OnlyTopDown = false;
}

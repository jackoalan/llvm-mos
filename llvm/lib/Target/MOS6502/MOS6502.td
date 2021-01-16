include "llvm/Target/Target.td"
include "llvm/Target/GlobalISel/Combine.td"

include "MOS6502RegisterInfo.td"

include "MOS6502CallingConv.td"
include "MOS6502Combine.td"
include "MOS6502RegisterBanks.td"
include "MOS6502InstrInfo.td"

def MOS6502SchedModel : SchedMachineModel {
  // Prevent instructions from being added to the pending queue, since latency
  // doesn't matter. The remaining latency concerns are disabled in the C++
  // scheduling strategy.
  let MicroOpBufferSize = 1;

  let CompleteModel = false;
}

def : ProcessorModel<"generic", MOS6502SchedModel, []>;

def MOS6502InstrInfo : InstrInfo {
  let guessInstructionProperties = 0;
}

def MOS6502 : Target {
  let InstructionSet = MOS6502InstrInfo;
  let AllowRegisterRenaming = 1;
}

include "llvm/Target/Target.td"
include "llvm/Target/GlobalISel/Combine.td"

include "MOS6502RegisterInfo.td"

include "MOS6502CallingConv.td"
include "MOS6502Combine.td"
include "MOS6502RegisterBanks.td"
include "MOS6502InstrInfo.td"

def : ProcessorModel<"generic", NoSchedModel, []>;

def MOS6502InstrInfo : InstrInfo {
  let guessInstructionProperties = 0;
}

def MOS6502 : Target {
  let InstructionSet = MOS6502InstrInfo;
}

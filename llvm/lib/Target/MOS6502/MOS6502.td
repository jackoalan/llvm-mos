include "llvm/Target/Target.td"

include "MOS6502RegisterInfo.td"

include "MOS6502RegisterBanks.td"
include "MOS6502InstrInfo.td"
include "MOS6502InstructionSelector.td"

def : ProcessorModel<"generic", NoSchedModel, []>;

def MOS6502InstrInfo : InstrInfo {
  let guessInstructionProperties = 0;
}

def MOS6502 : Target {
  let InstructionSet = MOS6502InstrInfo;
}

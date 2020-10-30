include "llvm/Target/GlobalISel/Combine.td"

def MOS6502InstructionSelectorCombinerHelper :
  GICombinerHelper<"MOS6502InstructionSelectorCombinerHelper", []>;
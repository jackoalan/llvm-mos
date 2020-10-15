include "llvm/Target/Target.td"

def : ProcessorModel<"generic", NoSchedModel, []>;

def MOS6502 : Target;

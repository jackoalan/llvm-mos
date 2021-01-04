class MOS6502GenericInstruction : GenericInstruction {
  let Namespace = "MOS6502";
}

def G_LSHO : MOS6502GenericInstruction {
  let OutOperandList = (outs type0:$dst, type1:$carry_out);
  let InOperandList = (ins type0:$src);
  let hasSideEffects = false;
}

def G_LSHE : MOS6502GenericInstruction {
  let OutOperandList = (outs type0:$dst, type1:$carry_out);
  let InOperandList = (ins type0:$src, type1:$carry_in);
  let hasSideEffects = false;
}
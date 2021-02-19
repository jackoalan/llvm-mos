class MOSGenericInstruction : GenericInstruction {
  let Namespace = "MOS";
}

def G_LSHO : MOSGenericInstruction {
  let OutOperandList = (outs type0:$dst, type1:$carry_out);
  let InOperandList = (ins type0:$src);
  let hasSideEffects = false;
}

def G_LSHE : MOSGenericInstruction {
  let OutOperandList = (outs type0:$dst, type1:$carry_out);
  let InOperandList = (ins type0:$src, type1:$carry_in);
  let hasSideEffects = false;
}
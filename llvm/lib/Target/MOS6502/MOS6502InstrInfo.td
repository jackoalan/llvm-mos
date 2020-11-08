class Pseudo : Instruction {
  let Namespace = "MOS6502";
  let Uses = [];
  let Defs = [];
  let hasSideEffects = 0;
  let isPseudo = 1;
  let mayLoad = 0;
  let mayStore = 0;
}

def CONSTANT : Pseudo {
  dag InOperandList = (ins i8imm:$src);
  dag OutOperandList = (outs I8:$dst);
}
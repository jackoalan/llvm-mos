class Instr : Instruction {
  let Namespace = "MOS6502";
  let Uses = [];
  let Defs = [];
  let hasSideEffects = 0;
  let mayLoad = 0;
  let mayStore = 0;
}

class Pseudo : Instr {
  let isPseudo = 1;
}

def CONSTANT : Pseudo {
  dag InOperandList = (ins i8imm:$src);
  dag OutOperandList = (outs I8:$dst);
}

def RTS : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let isReturn = 1;
  let isBarrier = 1;
  let isTerminator = 1;
  let hasCtrlDep = 1;
}

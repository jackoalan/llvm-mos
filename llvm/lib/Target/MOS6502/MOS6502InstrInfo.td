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

def LDimm : Instr {
  dag InOperandList = (ins i8imm:$src);
  dag OutOperandList = (outs GPR:$dst);
  let AsmString = "LD$dst\t#$src";

  let isMoveImm = 1;
}

def RTS : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let AsmString = "RTS";

  let isReturn = 1;
  let isBarrier = 1;
  let isTerminator = 1;
  let hasCtrlDep = 1;
}

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

let isReMaterializable = 1 in {

def CMPimm : Instr {
  dag InOperandList = (ins GPR:$l, i8imm:$r);
  dag OutOperandList = (outs);
  let Defs = [P];

  let isCompare = true;
}

def LDimm : Instr {
  dag InOperandList = (ins i8imm:$src);
  dag OutOperandList = (outs GPR:$dst);
  let AsmString = "LD$dst\t#$src";
  let Pattern = [(set GPR:$dst, imm:$src)];

  let isMoveImm = 1;
}

let isMoveReg = 1, isAsCheapAsAMove = 1 in {

def TA_ : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs XY:$dst);
  let AsmString = "TA$dst";
  let Uses = [A];
}

def T_A : Instr {
  dag InOperandList = (ins XY:$src);
  dag OutOperandList = (outs);
  let AsmString = "T${src}A";
  let Defs = [A];
}

}

}

def PHA : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let AsmString = "PHA";
  let Uses = [A];
  let mayStore = 1;
}

def PLA : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let AsmString = "PLA";
  let Defs = [A];
  let mayLoad = 1;
}

def label : Operand<OtherVT>;

let isBarrier = 1, isTerminator = 1 in {

let isBranch = 1 in {

def BNE : Instr {
  dag InOperandList = (ins label:$tgt);
  dag OutOperandList = (outs);
  let AsmString = "BNE $tgt";
  let Uses = [P];
}

def JMP : Instr {
  dag InOperandList = (ins label:$tgt);
  dag OutOperandList = (outs);
  let AsmString = "JMP $tgt";
  let Pattern = [(br bb:$tgt)];
}

}

def RTS : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let AsmString = "RTS";

  let isReturn = 1;
}

}

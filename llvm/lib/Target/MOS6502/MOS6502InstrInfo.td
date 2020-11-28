class Instr : Instruction {
  let Namespace = "MOS6502";
  let Uses = [];
  let Defs = [P];
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

  let isCompare = true;
}

def IN_ : Instr {
  dag InOperandList = (ins XY:$src);
  dag OutOperandList = (outs XY:$dst);
  let Constraints = "$src = $dst";

  let AsmString = "IN$src";
  let Pattern = [(set XY:$dst, (add XY:$src, 1))];

  let isAdd = 1;
}

def LDimm : Instr {
  dag InOperandList = (ins i8imm:$src);
  dag OutOperandList = (outs GPR:$dst);

  let AsmString = "LD$dst\t#$src";
  let Pattern = [(set GPR:$dst, imm:$src)];

  let isMoveImm = 1;
}

def LDAyindir : Instr {
  dag InOperandList = (ins ZP_PTR:$addr);
  dag OutOperandList = (outs);

  let AsmString = "LDA\t($addr),Y";
  let Uses = [Y];
  let Defs = [A, P];

  let mayLoad = 1;
}

let isMoveReg = 1, isAsCheapAsAMove = 1 in {

def TA_ : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs XY:$dst);
  let Uses = [A];

  let AsmString = "TA$dst";
}

def T_A : Instr {
  dag InOperandList = (ins XY:$src);
  dag OutOperandList = (outs);
  let AsmString = "T${src}A";
  let Defs = [A, P];
}

}

}

def PHA : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let Uses = [A];
  let Defs = [];

  let AsmString = "PHA";

  let mayStore = 1;
}

def PLA : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let Defs = [A, P];

  let AsmString = "PLA";

  let mayLoad = 1;
}

def label : Operand<OtherVT>;

let isBarrier = 1, isTerminator = 1 in {

let isBranch = 1 in {

def BNE : Instr {
  dag InOperandList = (ins label:$tgt);
  dag OutOperandList = (outs);
  let Uses = [P];
  let Defs = [];

  let AsmString = "BNE $tgt";
}

def JMP : Instr {
  dag InOperandList = (ins label:$tgt);
  dag OutOperandList = (outs);
  let Defs = [];

  let AsmString = "JMP $tgt";

  let Pattern = [(br bb:$tgt)];
}

}

def RTS : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let Defs = [];

  let AsmString = "RTS";

  let isReturn = 1;
}

}

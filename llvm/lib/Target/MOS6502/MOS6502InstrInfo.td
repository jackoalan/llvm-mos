class Instr : Instruction {
  let Namespace = "MOS6502";
  let Uses = [];
  let Defs = [P];
  let hasSideEffects = 0;
  let mayLoad = 0;
  let mayStore = 0;
}

class Pseudo : Instr {
  let isPseudo = true;
}

// Note: Written using Ac instead of implicit use/def to allow TwoAddressPass to
// commute.
def ADC : Pseudo {
  dag InOperandList = (ins Ac:$l, ZP:$r);
  dag OutOperandList = (outs Ac:$sum);
  let Constraints = "$l = $sum";

  let Uses = [P];

  let isAdd = true;
  let isCommutable = true;
  let isReMaterializable = true;
}

def GETC : Pseudo {
  dag InOperandList = (ins);
  dag OutOperandList = (outs i1imm:$c);

  let Uses = [P];
  let Defs = [];
}

def SETC : Pseudo {
  dag InOperandList = (ins i1imm:$c);
  dag OutOperandList = (outs);

  let isMoveImm = true;
}

let isReMaterializable = true in {

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

  let isAdd = true;
}

def LDimm : Instr {
  dag InOperandList = (ins i8imm:$src);
  dag OutOperandList = (outs GPR:$dst);

  let AsmString = "LD$dst\t#$src";
  let Pattern = [(set GPR:$dst, imm:$src)];

  let isMoveImm = true;
}

def LDAyindir : Instr {
  dag InOperandList = (ins ZP_PTR:$addr);
  dag OutOperandList = (outs);

  let AsmString = "LDA\t($addr),Y";
  let Uses = [Y];
  let Defs = [A, P];

  let mayLoad = true;
}

let isAsCheapAsAMove = true in {

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

  let mayStore = true;
}

def PLA : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let Defs = [A, P];

  let AsmString = "PLA";

  let mayLoad = true;
}

def label : Operand<OtherVT>;

let isBarrier = true, isTerminator = true in {

let isBranch = true in {

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

  let isReturn = true;
}

}

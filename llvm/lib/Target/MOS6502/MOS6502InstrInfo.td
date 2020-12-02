class Instr : Instruction {
  let Namespace = "MOS6502";

  let Uses = [];
  let Defs = [NZ];

  let hasSideEffects = 0;
  let mayLoad = 0;
  let mayStore = 0;
}

class Pseudo : Instr {
  let isPseudo = true;
}

let isReMaterializable = true in {

def ADCimm : Instr {
  dag InOperandList = (ins i8imm:$r);
  dag OutOperandList = (outs);

  let Uses = [A, C];
  let Defs = [A, C, NZ];

  let AsmString = "ADC\t#$r";

  let isAdd = true;
  let isReMaterializable = true;
}

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

def LDCimm : Instr {
  dag InOperandList = (ins i1imm:$val);
  dag OutOperandList = (outs);

  let Defs = [C];
}

def LDimm : Instr {
  dag InOperandList = (ins i8imm:$val);
  dag OutOperandList = (outs GPR:$dst);

  let AsmString = "LD$dst\t#$val";
  let Pattern = [(set GPR:$dst, imm:$val)];

  let isMoveImm = true;
}

def LDzp : Instr {
  dag InOperandList = (ins ZP:$src);
  dag OutOperandList = (outs GPR:$dst);

  let AsmString = "LD$dst\t$src";
}

def LDAyindir : Instr {
  dag InOperandList = (ins ZP_PTR:$addr);
  dag OutOperandList = (outs);

  let Uses = [Y];
  let Defs = [A, NZ];

  let AsmString = "LDA\t($addr),Y";

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

  let Defs = [A, NZ];
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
  let Defs = [A, NZ];

  let AsmString = "PLA";

  let mayLoad = true;
}

def PHP : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let Uses = [P];
  let Defs = [];

  let AsmString = "PHP";

  let mayStore = true;
}

def PLP : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let Defs = [P];

  let AsmString = "PLP";

  let mayLoad = true;
}


def STzp : Instr {
  dag InOperandList = (ins GPR:$src);
  dag OutOperandList = (outs ZP:$dst);

  let Defs = [];

  let AsmString = "ST$src\t$dst";
}

def label : Operand<OtherVT>;

let isBarrier = true, isTerminator = true in {

let isBranch = true in {

def BNE : Instr {
  dag InOperandList = (ins label:$tgt);
  dag OutOperandList = (outs);
  let Uses = [NZ];
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

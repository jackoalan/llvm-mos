def label : Operand<OtherVT>;
def frame_index : Operand<OtherVT>;

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
  let isReMaterializable = true;
}

def IN_ : Instr {
  dag InOperandList = (ins XY:$src);
  dag OutOperandList = (outs XY:$dst);
  let Constraints = "$src = $dst";

  let AsmString = "IN$src";
  let Pattern = [(set XY:$dst, (add XY:$src, 1))];

  let isAdd = true;
  let isReMaterializable = true;
}

def LDCimm : Instr {
  dag InOperandList = (ins i1imm:$val);
  dag OutOperandList = (outs);

  let Defs = [C];

  let isReMaterializable = true;
}

// Load a value from the given hard stack offset. This instruction is used by the
// register allocator to emit spills, and such operations do not recompute
// liveness information for phsyical registers. Accordingly, this instruction
// must be considered to preserve all physical registers. The details of arranging
// this are deferred to pseudo expansion time.
def LDhs : Pseudo {
  dag InOperandList = (ins frame_index:$fi);
  dag OutOperandList = (outs Anyi8:$dst);

  let Defs = [];

  let mayLoad = true;
}

// Version of LDimm that promises not to overwrite NZ. This allows
// rematerializing LDimm with a possible PHP/PLP instead of spilling/loading the
// constant on the stack. The spill is very expensive, and would need to
// PHP/PLP anyways, so rematerialization is strictly better in this case.
def LDimm_preserve : Pseudo {
  dag InOperandList = (ins i8imm:$val);
  dag OutOperandList = (outs GPR:$dst);

  let Defs = [];

  let isMoveImm = true;
  let isReMaterializable = true;
}

def LDimm : Instr {
  dag InOperandList = (ins i8imm:$val);
  dag OutOperandList = (outs GPR:$dst);

  let AsmString = "LD$dst\t#$val";
  let Pattern = [(set GPR:$dst, imm:$val)];

  let isMoveImm = true;
  let isReMaterializable = true;
}

def LDzpr : Pseudo {
  dag InOperandList = (ins ZP:$src);
  dag OutOperandList = (outs GPR:$dst);

  let isReMaterializable = true;
}

def LDzp : Instr {
  dag InOperandList = (ins i8imm:$src);
  dag OutOperandList = (outs GPR:$dst);

  let AsmString = "LD$dst\tz:$src";

  let isReMaterializable = true;
  let mayLoad = true;
}

// $dst = *(addr + (unsigned char)idx);
// Sets NZ to undefined value, since the expansion may end with a restore of A.
def LDidx : Pseudo {
  dag InOperandList = (ins i16imm:$addr, XY:$idx);
  dag OutOperandList = (outs GPR:$dst);

  let isReMaterializable = true;
  let mayLoad = true;
}

def LDAidx : Instr {
  dag InOperandList = (ins i16imm:$addr, XY:$idx);
  dag OutOperandList = (outs);

  let Defs = [A, NZ];

  let AsmString = "LDA\t$addr,$idx";

  let isReMaterializable = true;
  let mayLoad = true;
}

def LDXidx : Instr {
  dag InOperandList = (ins i16imm:$addr);
  dag OutOperandList = (outs);

  let Uses = [Y];
  let Defs = [X, NZ];

  let AsmString = "LDX\t$addr,Y";

  let isReMaterializable = true;
  let mayLoad = true;
}

def LDYidx : Instr {
  dag InOperandList = (ins i16imm:$addr);
  dag OutOperandList = (outs);

  let Uses = [X];
  let Defs = [Y, NZ];

  let AsmString = "LDY\t$addr,X";

  let isReMaterializable = true;
  let mayLoad = true;
}

def LDAyindirr : Pseudo {
  dag InOperandList = (ins ZP_PTR:$addr);
  dag OutOperandList = (outs);

  let Uses = [Y];
  let Defs = [A, NZ];

  let mayLoad = true;
}

def LDAyindir : Instr {
  dag InOperandList = (ins i8imm:$addr);
  dag OutOperandList = (outs);

  let Uses = [Y];
  let Defs = [A, NZ];

  let AsmString = "LDA\t($addr),Y";

  let mayLoad = true;
}

def TA_ : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs XY:$dst);

  let Uses = [A];

  let AsmString = "TA$dst";

  let isAsCheapAsAMove = true;
  let isReMaterializable = true;
}

def T_A : Instr {
  dag InOperandList = (ins XY:$src);
  dag OutOperandList = (outs);
  let AsmString = "T${src}A";

  let Defs = [A, NZ];

  let isAsCheapAsAMove = true;
  let isReMaterializable = true;
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

// Store a value to the given hard stack offset. This instruction is used by the
// register allocator to emit spills, and such operations do not recompute
// liveness information for phsyical registers. Accordingly, this instruction
// must be considered to preserve all physical registers. The details of arranging
// this are deferred to pseudo expansion time.
def SThs : Pseudo {
  dag InOperandList = (ins Anyi8:$src, frame_index:$fi);
  dag OutOperandList = (outs);

  let Defs = [];

  let mayStore = true;
}

def STzpr : Pseudo {
  dag InOperandList = (ins GPR:$src);
  dag OutOperandList = (outs ZP:$dst);

  let Defs = [];
}

def STzp : Instr {
  dag InOperandList = (ins i8imm:$dst, GPR:$src);
  dag OutOperandList = (outs);

  let Defs = [];

  let AsmString = "ST$src\tz:$dst";

  let mayStore = true;
}

// Branches to $tgt if $flag = $val.
def BR : Instr {
  dag InOperandList = (ins label:$tgt, Flag:$flag, i1imm:$val);
  dag OutOperandList = (outs);
  let Defs = [];

  let isBranch = true;
  let isTerminator = true;
}

def JMP : Instr {
  dag InOperandList = (ins label:$tgt);
  dag OutOperandList = (outs);

  let Defs = [];

  let AsmString = "JMP\t$tgt";

  let Pattern = [(br bb:$tgt)];

  let isBarrier = true;
  let isBranch = true;
  let isTerminator = true;
}

def JSR : Instr {
  dag InOperandList = (ins label:$tgt);
  dag OutOperandList = (outs);

  let Defs = [];

  let AsmString = "JSR\t$tgt";

  let isCall = true;
}

def RTS : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let Defs = [];

  let AsmString = "RTS";

  let isBarrier = true;
  let isReturn = true;
  let isTerminator = true;
}

//===-- MOSInstrInfo.td - MOS Instruction defs -------------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the MOS instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

/// These instruction format definitions exist, thanks to Neil Parker's 
/// analysis of the 65xx instruction set, at:
/// http://nparker.llx.com/a2/opcodes.html
/// Parker's analysis is closely reflected in the structure of this file, and
/// his words are quoted liberally herein.  Parker's sense-making of the 6502
/// and family was critical in creating these format definitions, and we're
/// indebted to him for the excellent analysis.

include "MOSInstrFormats.td"

let Predicates = [Has6502] in 
{
defm ORA : CC1_All< 0b000, (outs), (ins), "ora", []>;
defm AND : CC1_All< 0b001, (outs), (ins), "and", []>;
defm EOR : CC1_All< 0b010, (outs), (ins), "eor", []>;
defm ADC : CC1_All< 0b011, (outs), (ins), "adc", []>;
defm LDA : CC1_All< 0b101, (outs), (ins), "lda", []>;
defm CMP : CC1_All< 0b110, (outs), (ins), "cmp", []>;
defm SBC : CC1_All< 0b111, (outs), (ins), "sbc", []>;

/// The STA # instruction would be nonsensical, so skip it.
defm STA : CC1_NoImmediate< 0b100, (outs), (ins), "sta", []>;

/// CC2 regular instructions
defm ASL : CC2_Shift< 0b000, (outs), (ins), "asl", []>;
defm ROL : CC2_Shift< 0b001, (outs), (ins), "rol", []>;
defm LSR : CC2_Shift< 0b010, (outs), (ins), "lsr", []>;
defm ROR : CC2_Shift< 0b011, (outs), (ins), "ror", []>;

def : InstAlias<"asl a", (ASL_Accumulator)>;
def : InstAlias<"rol a", (ROL_Accumulator)>;
def : InstAlias<"lsr a", (LSR_Accumulator)>;
def : InstAlias<"ror a", (ROR_Accumulator)>;

defm DEC : CC2_NonShift< 0b110, (outs), (ins), "dec", []>;
defm INC : CC2_NonShift< 0b111, (outs), (ins), "inc", []>;

/// CC2 exceptional instructions
///             ASL ROL LSR ROR STX LDX DEC INC
/// #                               A2	 	 
/// zp          06  26  46  66  86  A6  C6  E6
/// A           0A  2A  4A  6A	 	 	 	 
/// abs         0E  2E  4E  6E  8E  AE  CE  EE
/// zp,X/zp,Y   16  36  56  76  96  B6  D6  F6
/// abs,X/abs,Y	1E	3E	5E	7E      BE  DE  FE

def STX_ZeroPage :
	Inst16<(outs), (ins), "stx", [], OpcodeC2<0b100, 0b001>, ZeroPage>;
def STX_ZeroPageY :
	Inst16<(outs), (ins), "stx", [], OpcodeC2<0b100, 0b101>, ZeroPageY>;
def STX_Absolute :
	Inst24<(outs), (ins), "stx", [], OpcodeC2<0b100, 0b011>, Absolute>;

def LDX_Immediate : 
	Inst16<(outs), (ins), "ldx", [], OpcodeC2<0b101, 0b000>, Immediate>;
def LDX_ZeroPage : 
	Inst16<(outs), (ins), "ldx", [], OpcodeC2<0b101, 0b001>, ZeroPage>;
def LDX_Absolute : 
	Inst24<(outs), (ins), "ldx", [], OpcodeC2<0b101, 0b011>, Absolute>;
def LDX_ZeroPageY : 
	Inst16<(outs), (ins), "ldx", [], OpcodeC2<0b101, 0b101>, ZeroPageY>;
def LDX_AbsoluteY : 
	Inst24<(outs), (ins), "ldx", [], OpcodeC2<0b101, 0b111>, AbsoluteY>;

def DEC_AbsoluteX : 
	Inst24<(outs), (ins), "dec", [], OpcodeC2<0b110, 0b111>, AbsoluteX>;
def INC_AbsoluteX : 
	Inst24<(outs), (ins), "inc", [], OpcodeC2<0b111, 0b111>, AbsoluteX>;


/// CC0 regular instructions, the nine in the top left corner of this diagram:
///       BIT JMP JMP() STY LDY CPY CPX
/// #                       A0  C0  E0
/// zp    24            84  A4  C4  E4
/// abs   2C  4C  6C    8C  AC  CC  EC
/// zp,X                94  B4	 	 
/// abs,X                   BC	 	
///
/// 101 LDY
/// 110 CPY
/// 111 CPX

defm LDY : CC0_Regular< 0b101, (outs), (ins), "ldy", []>;
defm CPY : CC0_Regular< 0b110, (outs), (ins), "cpy", []>;
defm CPX : CC0_Regular< 0b111, (outs), (ins), "cpx", []>;

/// CC0 irregular instructions, e.g. most of them

/// "Next, the cc = 00 instructions. Again, the opcodes are different:
/// aaa opcode
/// 001 BIT
/// 010 JMP
/// 011 JMP (abs)
/// 100 STY
/// 101 LDY
/// 110 CPY
/// 111 CPX
/// The addressing modes are the same as the 10 case, except that accumulator
/// mode is missing.
/// bbb addressing mode
/// 000 #immediate
/// 001 zero page
/// 011 absolute
/// 101 zero page,X
/// 111 absolute,X
/// And here's how they fit together:
///
///       BIT JMP JMP() STY LDY CPY CPX
/// #                       A0  C0  E0
/// zp    24            84  A4  C4  E4
/// abs   2C  4C  6C    8C  AC  CC  EC
/// zp,X                94  B4	 	 
/// abs,X                   BC	 	 

def BIT_ZeroPage : 
	Inst16<(outs), (ins), "bit", [], OpcodeC0<0b001, 0b001>, ZeroPage>;
def BIT_Absolute : 
	Inst24<(outs), (ins), "bit", [], OpcodeC0<0b001, 0b011>, Absolute>;
def JMP_Absolute : 
	Inst24<(outs), (ins), "jmp", [], OpcodeC0<0b010, 0b011>, Absolute>;
def JMP_Indirect : 
	Inst24<(outs), (ins), "jmp", [], OpcodeC0<0b011, 0b011>, Indirect>;
def STY_ZeroPage :
	Inst16<(outs), (ins), "sty", [], OpcodeC0<0b100, 0b001>, ZeroPage>;
def STY_Absolute : 
	Inst24<(outs), (ins), "sty", [], OpcodeC0<0b100, 0b011>, Absolute>;
def STY_ZeroPageX : 
	Inst16<(outs), (ins), "sty", [], OpcodeC0<0b100, 0b101>, ZeroPageX>;
def LDY_ZeroPageX : 
	Inst16<(outs), (ins), "ldy", [], OpcodeC0<0b101, 0b101>, ZeroPageX>;
def LDY_AbsoluteX : 
	Inst24<(outs), (ins), "ldy", [], OpcodeC0<0b101, 0b111>, AbsoluteX>;

/// The conditional branch instructions all have the form xxy10000. The flag
/// indicated by xx is compared with y, and the branch is taken if they are
/// equal.
/// xx flag
/// 00 negative
/// 01 overflow
/// 10 carry
/// 11 zero
/// This gives the following branches:
/// BPL BMI BVC BVS BCC BCS BNE BEQ
/// 10  30  50  70  90  B0  D0  F0

def BPL_Relative : ConditionalBranch< "bpl", (outs), (ins), [], 0b00, 0b0 >;
def BMI_Relative : ConditionalBranch< "bmi", (outs), (ins), [], 0b00, 0b1 >;
def BVC_Relative : ConditionalBranch< "bvc", (outs), (ins), [], 0b01, 0b0 >;
def BVS_Relative : ConditionalBranch< "bvs", (outs), (ins), [], 0b01, 0b1 >;
def BCC_Relative : ConditionalBranch< "bcc", (outs), (ins), [], 0b10, 0b0 >;
def BCS_Relative : ConditionalBranch< "bcs", (outs), (ins), [], 0b10, 0b1 >;
def BNE_Relative : ConditionalBranch< "bne", (outs), (ins), [], 0b11, 0b0 >;
def BEQ_Relative : ConditionalBranch< "beq", (outs), (ins), [], 0b11, 0b1 >;

/// Single-byte opcodes
/// BRK JSR abs RTI RTS
/// 00  20      40  60
/// (JSR is the only absolute-addressing instruction that doesn't fit the
/// aaabbbcc pattern.)
///
/// Other single-byte instructions:
/// PHP PLP PHA PLA DEY TAY INY INX
/// 08  28  48  68  88  A8  C8  E8
/// CLC SEC CLI SEI TYA CLV CLD SED
/// 18  38  58  78  98  B8  D8  F8
/// TXA TXS TAX TSX DEX NOP
/// 8A  9A  AA  BA  CA  EA

def BRK_Implied: InstLow0<(outs), (ins), "brk", [], 0b0000>;
/// JSR is the only instruction that does not follow the current pattern.
def JSR_Absolute: Inst24< (outs), (ins), "jsr", [], 
						Opcode< 0x20 >, Absolute >;
def RTI_Implied: InstLow0<(outs), (ins), "rti", [], 0b0100>;
def RTS_Implied: InstLow0<(outs), (ins), "rts", [], 0b0110>;

def PHP_Implied: InstLow8<(outs), (ins), "php", [], 0b0000>;
def PLP_Implied: InstLow8<(outs), (ins), "plp", [], 0b0010>;
def PHA_Implied: InstLow8<(outs), (ins), "pha", [], 0b0100>;
def PLA_Implied: InstLow8<(outs), (ins), "pla", [], 0b0110>;
def DEY_Implied: InstLow8<(outs), (ins), "dey", [], 0b1000>;
def TAY_Implied: InstLow8<(outs), (ins), "tay", [], 0b1010>;
def INY_Implied: InstLow8<(outs), (ins), "iny", [], 0b1100>;
def INX_Implied: InstLow8<(outs), (ins), "inx", [], 0b1110>;

def CLC_Implied: InstLow8<(outs), (ins), "clc", [], 0b0001>;
def SEC_Implied: InstLow8<(outs), (ins), "sec", [], 0b0011>;
def CLI_Implied: InstLow8<(outs), (ins), "cli", [], 0b0101>;
def SEI_Implied: InstLow8<(outs), (ins), "sei", [], 0b0111>;
def TYA_Implied: InstLow8<(outs), (ins), "tya", [], 0b1001>;
def CLV_Implied: InstLow8<(outs), (ins), "clv", [], 0b1011>;
def CLD_Implied: InstLow8<(outs), (ins), "cld", [], 0b1101>;
def SED_Implied: InstLow8<(outs), (ins), "sed", [], 0b1111>;

def TXA_Implied: InstLowA<(outs), (ins), "txa", [], 0b1000>;    
def TXS_Implied: InstLowA<(outs), (ins), "txs", [], 0b1001>;
def TAX_Implied: InstLowA<(outs), (ins), "tax", [], 0b1010>;
def TSX_Implied: InstLowA<(outs), (ins), "tsx", [], 0b1011>;
def DEX_Implied: InstLowA<(outs), (ins), "dex", [], 0b1100>;
def NOP_Implied: InstLowA<(outs), (ins), "nop", [], 0b1110>;

} // Predicates = [Has6502]

include "MOSInstrInfoTables.td"

def label : Operand<OtherVT>;

class Instr : Instruction {
  let Namespace = "MOS";

  let Uses = [];
  let Defs = [NZ];

  let hasSideEffects = 0;
  let mayLoad = 0;
  let mayStore = 0;
}

// Pseudoinstructions lower to a sequence of real 6502 instructions. Which
// sequence of instructions they expand to usually depends on the register
// allocation of their operands.
//
// Unfortunately, it's often the case that the registers internally
// used/clobbered by these instruction sequences depends on the register
// allocation as well. Were these sequences emitted naively, they could clobber
// values used by future instructions.
//
// Broadly, there are two different locations where these pseudos can be emitted:
// before and during/after register allocation. Pseudos that can only be emitted
// before register allocation should give a conservative estimate of the real
// register uses of their expansions. Then the register allocator can later use
// this information to avoid unneccessary conflicts. For pseudos that can be
// emitted during/after register allocation, it's too late for the register
// allocator to do this, so pseudos must instead report that they affect
// nothing.
//
// In either case, once the pseudo instructions are to be lowered to a real
// sequence of instructions, register allocation has already occurred. The
// emitted instruction sequence is derived from the register allocation of the
// pseudo's operand. The sequence is then wrapped in appropriate save/reload
// code to make its register use/def profile match the one advertized by the
// pseudo definition in this file. It only need to match in an "as-if" fashion:
// any locations that are not live across the pseudo may be freely used without
// save/restore.

class Pseudo : Instr {
  let isPseudo = true;
}

// Frame Pseudoinstructions

def LDstk : Pseudo {
  dag InOperandList = (ins Stack:$base, i16imm:$offset);
  dag OutOperandList = (outs Anyc:$dst);

  let Defs = [];

  let mayLoad = true;
}

def STstk : Pseudo {
  dag InOperandList = (ins Anyc:$src, Stack:$base, i16imm:$fi);
  dag OutOperandList = (outs);

  let Defs = [];

  let mayStore = true;
}

// Produces the low byte of the address of the given frame index. Produces a
// carry required by the high byte calculation.
def AddrLostk : Pseudo {
  dag InOperandList = (ins Stack:$base, i16imm:$offset);
  dag OutOperandList = (outs Anyi8:$dst, Cc:$carry);

  let Defs = [NZ];

  let isReMaterializable = true;
}

// Produces the high byte of the address of the given frame index. Accepts the
// carry produced by the low byte calculation.
def AddrHistk : Pseudo {
  dag InOperandList = (ins Stack:$base, i16imm:$offset, Cc:$carry);
  dag OutOperandList = (outs Anyi8:$dst);

  let Defs = [C, NZ];

  let isReMaterializable = true;
}

// Produces the address of the given frame index. Doesn't allow instructions
// to be inserted between calculation of the low and high byte, but in exchange,
// the instruction can be moved around and rematerialized as a whole.
def Addrstk : Pseudo {
  dag InOperandList = (ins Stack:$base, i16imm:$offset);
  dag OutOperandList = (outs ZP_PTR:$dst);

  let Defs = [NZ];

  let isReMaterializable = true;
}

// Hard Stack

def Pull : Pseudo {
  dag InOperandList = (ins);
  dag OutOperandList = (outs Anyi8:$dst);

  let Defs = [];

  let mayLoad = true;
}

// Same as pull, but preserving the value in the A register if live.
def PullPreserve : Pseudo {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);

  let Defs = [];

  let mayLoad = true;
}

def Push : Pseudo {
  dag InOperandList = (ins Anyi8:$src);
  dag OutOperandList = (outs);

  let Defs = [];

  let mayStore = true;
}

// Soft stack

// Add $bytes bytes to SP, saving the result back in SP. A negative argument
// decreases SP.
def IncSP : Pseudo {
  dag InOperandList = (ins i16imm:$bytes);
  dag OutOperandList = (outs);

  let Uses = [SP];
  let Defs = [SP];
}

def ADJCALLSTACKDOWN : Pseudo {
  dag InOperandList = (ins i16imm:$bytes, i16imm:$prior);
  dag OutOperandList = (outs);

  let Uses = [SP];
  let Defs = [SP];
}

def ADJCALLSTACKUP : Pseudo {
  dag InOperandList = (ins i16imm:$bytes, i16imm:$prior);
  dag OutOperandList = (outs);

  let Uses = [SP];
  let Defs = [SP];
}

// Miscellaneous pseudoinstructions

// $dst = *(addr + (unsigned char)idx);
// Sets NZ to undefined value, since expansion may clobber.
def LDidx : Pseudo {
  dag InOperandList = (ins i16imm:$addr, XY:$idx);
  dag OutOperandList = (outs GPR:$dst);

  let isReMaterializable = true;
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

// Regular Instructions

def ADCimm : Instr {
  dag InOperandList = (ins Ac:$l, i8imm:$r, Cc:$carryin);
  dag OutOperandList = (outs Ac:$dst, Cc:$carryout);
  let Constraints = "$dst = $l, $carryout = $carryin";

  let Defs = [NZ];

  let AsmString = "ADC\t#$r";

  let isAdd = true;
  let isReMaterializable = true;
}

def ADCzpr : Instr {
  dag InOperandList = (ins Ac:$l, ZP:$r, Cc:$carryin);
  dag OutOperandList = (outs Ac:$dst, Cc:$carryout);
  // Don't give the carries a tied constraint, since having two tied operands
  // inhibits commutation of $l and $r.
  let Constraints = "$dst = $l";

  let Defs = [NZ];

  let AsmString = "ADC\tz:$r";

  let isAdd = true;
  let isCommutable = true;
  let isReMaterializable = true;
}

def ASL : Instr {
  dag InOperandList = (ins AZP:$src);
  dag OutOperandList = (outs AZP:$dst, Cc:$carry);
  let Constraints = "$dst = $src";

  let Defs = [NZ];

  let AsmString = "ASL\t$dst";

  let isReMaterializable = true;
}

def CMPimm : Instr {
  dag InOperandList = (ins GPR:$l, i8imm:$r);
  dag OutOperandList = (outs);

  let isCompare = true;
  let isReMaterializable = true;

  // Keep CMPimm adjacent to the branch that uses it.
  // This prevents NZ from ever being live across an instruction.
  let isTerminator = true;
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
  dag OutOperandList = (outs Cc:$dst);

  let Defs = [];

  let isReMaterializable = true;
  let isAsCheapAsAMove = true;
}

def LDimm : Instr {
  dag InOperandList = (ins i8imm:$val);
  dag OutOperandList = (outs GPR:$dst);

  let AsmString = "LD$dst\t#$val";
  let Pattern = [(set GPR:$dst, imm:$val)];

  let isAsCheapAsAMove = true;
  let isMoveImm = true;
  let isReMaterializable = true;
}

def LDzpr : Instr {
  dag InOperandList = (ins ZP:$src);
  dag OutOperandList = (outs GPR:$dst);

  let AsmString = "LD$dst\tz:$src";

  let isReMaterializable = true;
}

def LDabs : Instr {
  dag InOperandList = (ins i16imm:$src);
  dag OutOperandList = (outs GPR:$dst);

  let AsmString = "LD$dst\t$src";

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

def LDyindirr : Instr {
  dag InOperandList = (ins ZP_PTR:$addr, Yc:$offset);
  dag OutOperandList = (outs Ac:$dst);

  let Defs = [NZ];

  let AsmString = "LDA\t($addr),Y";

  let mayLoad = true;
}

def ROL : Instr {
  dag InOperandList = (ins AZP:$src, Cc:$carryin);
  dag OutOperandList = (outs AZP:$dst, Cc:$carryout);
  let Constraints = "$dst = $src, $carryout = $carryin";

  let Defs = [NZ];

  let AsmString = "ROL\t$dst";

  let isReMaterializable = true;
}

def SBCimm : Instr {
  dag InOperandList = (ins Ac:$l, i8imm:$r, Cc:$carryin);
  dag OutOperandList = (outs Ac:$dst, Cc:$carryout);
  let Constraints = "$dst = $l, $carryout = $carryin";

  let Defs = [NZ];

  let AsmString = "SBC\t#$r";

  let isReMaterializable = true;
}

def SBCzpr : Instr {
  dag InOperandList = (ins Ac:$l, ZP:$r, Cc:$carryin);
  dag OutOperandList = (outs Ac:$dst, Cc:$carryout);
  // Don't give the carries a tied constraint, since having two tied operands
  // inhibits commutation of $l and $r.
  let Constraints = "$dst = $l";

  let Defs = [NZ];

  let AsmString = "SBC\tz:$r";

  let isReMaterializable = true;
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

def TSX : Instr {
  dag InOperandList = (ins);
  dag OutOperandList = (outs);
  let AsmString = "TSX";

  let Defs = [X, NZ];

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

def STzpr : Instr {
  dag InOperandList = (ins GPR:$src);
  dag OutOperandList = (outs ZP:$dst);

  let AsmString = "ST$src\tz:$dst";

  let Defs = [];
}

def STabs : Instr {
  dag InOperandList = (ins GPR:$src, i16imm:$dst);
  dag OutOperandList = (outs);

  let Defs = [];

  let AsmString = "ST$src\t$dst";

  let mayStore = true;
}

def STidx : Instr {
  dag InOperandList = (ins Ac:$src, i16imm:$addr, XY:$idx);
  dag OutOperandList = (outs);

  let AsmString = "STA\t$addr,$idx";

  let mayStore = true;
}

def STyindirr : Instr {
  dag InOperandList = (ins Ac:$src, ZP_PTR:$addr, Yc:$offset);
  dag OutOperandList = (outs);

  let Defs = [];

  let AsmString = "STA\t($addr),Y";

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

include "MOSInstrGISel.td"


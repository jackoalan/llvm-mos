include "MOSInstrFormatsSPC700.td"

let Predicates = [HasSPC700], DecoderNamespace = "SPC700" in {

defm STA : SPC700_LoadStoreA<0b110, "sta">;
defm LDA : SPC700_LoadStoreA<0b111, "lda">;
defm LDA_Immediate : SPC700_Inst16<"lda", OpcodeC0<0b111, 0b010>, Immediate>;
defm LDX_ZeroPage : SPC700_Inst16<"ldx", OpcodeC0<0b111, 0b110>, ZeroPage>;
defm STX_ZeroPage : SPC700_Inst16<"stx", OpcodeC0<0b110, 0b110>, ZeroPage>;
defm LDY_Absolute : SPC700_Inst24<"ldy", OpcodeC0<0b111, 0b011>, Absolute>;
defm STY_Absolute : SPC700_Inst24<"sty", OpcodeC0<0b110, 0b011>, Absolute>;
defm LDX_Absolute : SPC700_Inst24<"ldx", OpcodeC1<0b111, 0b010>, Absolute>;
defm STX_Absolute : SPC700_Inst24<"stx", OpcodeC1<0b110, 0b010>, Absolute>;
defm LDX_ZeroPageY : SPC700_Inst16<"ldx", OpcodeC1<0b111, 0b110>, ZeroPageY>;
defm STX_ZeroPageY : SPC700_Inst16<"stx", OpcodeC1<0b110, 0b110>, ZeroPageY>;
defm LDX_Immediate : SPC700_Inst16<"ldx", OpcodeC1<0b110, 0b011>, Immediate>;
defm LDY_Immediate : SPC700_Inst16<"ldy", OpcodeC1<0b100, 0b011>, Immediate>;
defm LDY_ZeroPageX : SPC700_Inst16<"ldy", OpcodeC3<0b111, 0b110>, ZeroPageX>;
defm STY_ZeroPageX : SPC700_Inst16<"sty", OpcodeC3<0b110, 0b110>, ZeroPageX>;
defm LDY_ZeroPage : SPC700_Inst16<"ldy", OpcodeC3<0b111, 0b010>, ZeroPage>;
defm STY_ZeroPage : SPC700_Inst16<"sty", OpcodeC3<0b110, 0b010>, ZeroPage>;

defm ORA : SPC700_Arithmetic<0b000, "ora">;
defm AND : SPC700_Arithmetic<0b001, "and">;
defm EOR : SPC700_Arithmetic<0b010, "eor">;
defm CMP : SPC700_Arithmetic<0b011, "cmp">;
defm ADC : SPC700_Arithmetic<0b100, "adc">;
defm SBC : SPC700_Arithmetic<0b101, "sbc">;

defm ASL : SPC700_Shift<0b000, "asl">;
defm ROL : SPC700_Shift<0b001, "rol">;
defm LSR : SPC700_Shift<0b010, "lsr">;
defm ROR : SPC700_Shift<0b011, "ror">;
defm DEC : SPC700_DecInc<0b100, "dec">;
def SPC700_DEC_Accumulator :
  Inst8<"dec", OpcodeC0<0b100, 0b111>, Accumulator>;
defm INC : SPC700_DecInc<0b101, "inc">;
def SPC700_INC_Accumulator :
  Inst8<"inc", OpcodeC0<0b101, 0b111>, Accumulator>;

defm BPL_Relative : SPC700_ConditionalBranch<0b000, "bpl">;
defm BMI_Relative : SPC700_ConditionalBranch<0b001, "bmi">;
defm BVC_Relative : SPC700_ConditionalBranch<0b010, "bvc">;
defm BVS_Relative : SPC700_ConditionalBranch<0b011, "bvs">;
defm BCC_Relative : SPC700_ConditionalBranch<0b100, "bcc">;
defm BCS_Relative : SPC700_ConditionalBranch<0b101, "bcs">;
defm BNE_Relative : SPC700_ConditionalBranch<0b110, "bne">;
defm BEQ_Relative : SPC700_ConditionalBranch<0b111, "beq">;
defm BRA_Relative : SPC700_Inst16<"bra", OpcodeC3<0b001, 0b011>, Relative>;

defm JSR_Absolute :
  SPC700_Inst24<"jsr", OpcodeC3<0b001, 0b111>, Absolute>;
defm JMP_Absolute :
  SPC700_Inst24<"jmp", OpcodeC3<0b010, 0b111>, Absolute>;
def SPC700_JMP_AbsoluteIndexedIndirect :
  Inst24<"jmp", OpcodeC3<0b000, 0b111>, AbsoluteIndexedIndirect>;
def : MOSToSPC700MappingEntry<JMP_Indirect,
                              SPC700_JMP_AbsoluteIndexedIndirect>;

defm RTI_Implied : SPC700_Inst8<"rti", OpcodeC3<0b011, 0b111>>;
defm RTS_Implied : SPC700_Inst8<"rts", OpcodeC3<0b011, 0b011>>;

defm CPX_Immediate : SPC700_Inst16<"cpx", OpcodeC0<0b110, 0b010>, Immediate>;
defm CPX_Absolute : SPC700_Inst24<"cpx", OpcodeC2<0b000, 0b111>, Absolute>;
defm CPX_ZeroPage : SPC700_Inst16<"cpx", OpcodeC2<0b001, 0b111>, ZeroPage>;
defm CPY_Immediate : SPC700_Inst16<"cpy", OpcodeC1<0b101, 0b011>, Immediate>;
defm CPY_Absolute : SPC700_Inst24<"cpy", OpcodeC2<0b010, 0b111>, Absolute>;
defm CPY_ZeroPage : SPC700_Inst16<"cpy", OpcodeC2<0b011, 0b111>, ZeroPage>;

defm NOP_Implied : SPC700_FlagOp<0b000, "nop">;
def SPC700_CLP_Implied : Inst8<"clp", OpcodeC0<0b001, 0b000>>;
def SPC700_SEP_Implied : Inst8<"sep", OpcodeC0<0b010, 0b000>>;
defm CLC_Implied : SPC700_FlagOp<0b011, "clc">;
defm SEC_Implied : SPC700_FlagOp<0b100, "sec">;
defm CLI_Implied : SPC700_FlagOp<0b101, "cli">;
defm SEI_Implied : SPC700_FlagOp<0b110, "sei">;
defm CLV_Implied : SPC700_FlagOp<0b111, "clv">;

defm PLP_Implied : SPC700_Inst8<"plp", OpcodeC2<0b100, 0b011>>;
defm PLA_Implied : SPC700_Inst8<"pla", OpcodeC2<0b101, 0b011>>;
defm PLX_Implied : SPC700_Inst8<"plx", OpcodeC2<0b110, 0b011>>;
defm PLY_Implied : SPC700_Inst8<"ply", OpcodeC2<0b111, 0b011>>;

defm PHP_Implied : SPC700_Inst8<"php", OpcodeC1<0b000, 0b011>>;
defm PHA_Implied : SPC700_Inst8<"pha", OpcodeC1<0b001, 0b011>>;
defm PHX_Implied : SPC700_Inst8<"phx", OpcodeC1<0b010, 0b011>>;
defm PHY_Implied : SPC700_Inst8<"phy", OpcodeC1<0b011, 0b011>>;

defm DEY_Implied : SPC700_Inst8<"dey", OpcodeC0<0b110, 0b111>>;
defm INY_Implied : SPC700_Inst8<"iny", OpcodeC0<0b111, 0b111>>;
defm DEX_Implied : SPC700_Inst8<"dex", OpcodeC1<0b000, 0b111>>;
defm INX_Implied : SPC700_Inst8<"inx", OpcodeC1<0b001, 0b111>>;
defm TAX_Implied : SPC700_Inst8<"tax", OpcodeC1<0b010, 0b111>>;
defm TXA_Implied : SPC700_Inst8<"txa", OpcodeC1<0b011, 0b111>>;
defm TSX_Implied : SPC700_Inst8<"tsx", OpcodeC1<0b100, 0b111>>;
defm TXS_Implied : SPC700_Inst8<"txs", OpcodeC1<0b101, 0b111>>;
defm TYA_Implied : SPC700_Inst8<"tya", OpcodeC1<0b110, 0b111>>;
defm TAY_Implied : SPC700_Inst8<"tay", OpcodeC1<0b111, 0b111>>;

}

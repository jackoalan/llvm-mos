class MOSToSPC700MappingEntry<InstAddressMode from,
                              InstAddressMode to> {
  InstAddressMode From = from;
  InstAddressMode To = to;
  list<Predicate> Predicates = [];
  string DecoderNamespace = "";
}

class MOS2SPC<InstAddressMode from> :
  MOSToSPC700MappingEntry<from, !cast<InstAddressMode>("SPC700_"#from)>;

multiclass SPC700_Inst8<string asmstr, Opcode op = DefaultOpcode,
                        AddressingMode mode = Implicit> {
  def SPC700_#NAME : Inst8<asmstr, op, mode>;
  def : MOS2SPC<!cast<InstAddressMode>(NAME)>;
}

multiclass SPC700_Inst16<string asmstr, Opcode op = DefaultOpcode,
                         AddressingMode mode = Implicit> {
  def SPC700_#NAME : Inst16<asmstr, op, mode>;
  def : MOS2SPC<!cast<InstAddressMode>(NAME)>;
}

multiclass SPC700_Inst24<string asmstr, Opcode op = DefaultOpcode,
                         AddressingMode mode = Implicit> {
  def SPC700_#NAME : Inst24<asmstr, op, mode>;
  def : MOS2SPC<!cast<InstAddressMode>(NAME)>;
}

/// Only used by SPC700_JMP_AbsoluteIndexedIndirect
def AbsoluteIndexedIndirect : AddressingMode {
  let OperandsStr = "( $param , x )";
  let InOperandList = (ins addr16:$param);
}

multiclass SPC700_LoadStoreA<bits<3> aaa, string OpcodeStr = "nop"> {
/// For cc = 00, then the meaning of bbb is:
/// 001 zero page
/// 101 zero page,X

  defm _ZeroPage :
    SPC700_Inst16<OpcodeStr, OpcodeC0<aaa, 0b001>, ZeroPage>;

  defm _ZeroPageX :
    SPC700_Inst16<OpcodeStr, OpcodeC0<aaa, 0b101>, ZeroPageX>;

/// For cc = 10, then the meaning of bbb is:
/// TODO: 001 (X)
/// 101 absolute,Y

  defm _AbsoluteY :
    SPC700_Inst24<OpcodeStr, OpcodeC2<aaa, 0b101>, AbsoluteY>;

/// For cc = 01, then the meaning of bbb is:
/// TODO: 010 dd,ds
/// TODO: 110 (X), (Y)
/// 001 absolute
/// 101 absolute,X

  defm _Absolute :
    SPC700_Inst24<OpcodeStr, OpcodeC1<aaa, 0b001>, Absolute>;

  defm _AbsoluteX :
    SPC700_Inst24<OpcodeStr, OpcodeC1<aaa, 0b101>, AbsoluteX>;

/// For cc = 11, then the meaning of bbb is:
/// 001 (zero page,X)
/// 101 (zero page),Y

  defm _IndexedIndirect :
    SPC700_Inst16<OpcodeStr, OpcodeC3<aaa, 0b001>, IndexedIndirect>;

  defm _IndirectIndexed :
    SPC700_Inst16<OpcodeStr, OpcodeC3<aaa, 0b101>, IndirectIndexed>;
}

multiclass SPC700_Arithmetic<bits<3> aaa, string OpcodeStr = "nop"> :
  SPC700_LoadStoreA<aaa, OpcodeStr> {
/// For cc = 00, then the meaning of bbb is:
/// 010 #immediate
/// TODO: 110 zero page,#immediate

  defm _Immediate : SPC700_Inst16<OpcodeStr, OpcodeC0<aaa, 0b010>, Immediate>;
}

multiclass SPC700_DecInc<bits<3> aaa, string OpcodeStr = "nop"> {
/// For cc = 00, then the meaning of bbb is:
/// 011 absolute

  defm _Absolute :
    SPC700_Inst24<OpcodeStr, OpcodeC0<aaa, 0b011>, Absolute>;

/// For cc = 11, then the meaning of bbb is:
/// 010 zero page
/// 110 zero page,X

  defm _ZeroPage :
    SPC700_Inst16<OpcodeStr, OpcodeC3<aaa, 0b010>, ZeroPage>;

  defm _ZeroPageX :
    SPC700_Inst16<OpcodeStr, OpcodeC3<aaa, 0b110>, ZeroPageX>;
}

multiclass SPC700_Shift<bits<3> aaa, string OpcodeStr = "nop"> :
  SPC700_DecInc<aaa, OpcodeStr> {
/// For cc = 00, then the meaning of bbb is:
/// 111 accumulator

  defm _Accumulator :
    SPC700_Inst8<OpcodeStr, OpcodeC0<aaa, 0b111>, Accumulator>;
}

multiclass SPC700_ConditionalBranch<bits<3> aaa, string OpcodeStr = "nop"> :
  SPC700_Inst16<OpcodeStr, OpcodeC0<aaa, 0b100>, Relative>;

multiclass SPC700_FlagOp<bits<3> aaa, string OpcodeStr = "nop"> :
  SPC700_Inst8<OpcodeStr, OpcodeC0<aaa, 0b000>>;

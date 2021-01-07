let Namespace = "MOS6502" in {

def sublo : SubRegIndex<8>;
def subhi : SubRegIndex<8>;
def subcarry : SubRegIndex<1>;
def subn : SubRegIndex<2>;
def subz : SubRegIndex<2>;

}

class MOS6502Reg<string n> : Register<n> {
  let Namespace = "MOS6502";
}

def A : MOS6502Reg<"A">;
def X : MOS6502Reg<"X">;
def Y : MOS6502Reg<"Y">;

def S : MOS6502Reg<"S">;

def C : MOS6502Reg<"C">;
def N : MOS6502Reg<"N">;
def Z : MOS6502Reg<"Z">;
def P : MOS6502Reg<"P"> {
  let SubRegs = [C, N, Z];
  let SubRegIndices = [subcarry, subn, subz];
}
def NZ : MOS6502Reg<"NZ"> {
  let SubRegs = [N, Z];
  let SubRegIndices = [subn, subz];
}

def SPlo : MOS6502Reg<"SPlo">;
def SPhi : MOS6502Reg<"SPhi">;

foreach i = 0 ... 253 in {
  def ZP_#i : MOS6502Reg<"ZP_"#i>;
}

def SP : MOS6502Reg<"SP"> {
  let SubRegs = [SPlo, SPhi];
  let SubRegIndices = [sublo, subhi];
  let CoveredBySubRegs = true;
}

foreach i = 0 ... 126 in {
  defvar lo = !mul(i, 2);
  defvar hi = !add(lo, 1);

  def ZP_PTR_#i : MOS6502Reg<"ZP_PTR_"#i> {
    let SubRegs = [!cast<Register>("ZP_"#lo), !cast<Register>("ZP_"#hi)];
    let SubRegIndices = [sublo, subhi];
    let CoveredBySubRegs = true;
  }
}

class MOS6502RegClass<list<ValueType> regTypes, int alignment, dag regList>
     : RegisterClass<"MOS6502", regTypes, alignment, regList>;

def GPR : MOS6502RegClass<[i8], 8, (add A, X, Y)>;

def Flag : MOS6502RegClass<[i1], 8, (add C, N, Z)> {
  let isAllocatable = false;
}

let isPressureFineGrained = true in {
  def Ac : MOS6502RegClass<[i8], 8, (add A)>;
  def XY : MOS6502RegClass<[i8], 8, (add X, Y)>;
}

def ZP : MOS6502RegClass<[i8], 8, (add SPlo, SPhi, (sequence "ZP_%u", 0, 253))>;
def ZP_PTR : MOS6502RegClass<[i16], 8, (add SP, (sequence "ZP_PTR_%u", 0, 126))>;

def AZP : MOS6502RegClass<[i8], 8, (add A, ZP)>;

def Anyi8 : MOS6502RegClass<[i8], 8, (add GPR, ZP)>;
def Anyi1 : MOS6502RegClass<[i1], 8, (add GPR, ZP, C)>;

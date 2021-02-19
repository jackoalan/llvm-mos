//===-- MOSRegisterInfo.td - MOS Register defs -------------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
let Namespace = "MOS" in {

def sublo : SubRegIndex<8>;
def subhi : SubRegIndex<8>;
def subcarry : SubRegIndex<1>;
def subnz : SubRegIndex<2>;
def subn : SubRegIndex<1>;
def subz : SubRegIndex<1>;

}

class MOSReg<string n> : Register<n> {
  let Namespace = "MOS";
}

def A : MOSReg<"A">;
def X : MOSReg<"X">;
def Y : MOSReg<"Y">;

def C : MOSReg<"C">;
def N : MOSReg<"N">;
def Z : MOSReg<"Z">;
def NZ : MOSReg<"NZ"> {
  let SubRegs = [N, Z];
  let SubRegIndices = [subn, subz];
  let CoveredBySubRegs = true;
}
def P : MOSReg<"P"> {
  let SubRegs = [C, NZ];
  let SubRegIndices = [subcarry, subnz];
}

def SPlo : MOSReg<"SPlo">;
def SPhi : MOSReg<"SPhi">;

foreach i = 0 ... 253 in {
  def ZP_#i : MOSReg<"ZP_"#i>;
}

def S : MOSReg<"S">;

def SP : MOSReg<"SP"> {
  let SubRegs = [SPlo, SPhi];
  let SubRegIndices = [sublo, subhi];
  let CoveredBySubRegs = true;
}

def Static : MOSReg<"Static">;

foreach i = 0 ... 126 in {
  defvar lo = !mul(i, 2);
  defvar hi = !add(lo, 1);

  def ZP_PTR_#i : MOSReg<"ZP_PTR_"#i> {
    let SubRegs = [!cast<Register>("ZP_"#lo), !cast<Register>("ZP_"#hi)];
    let SubRegIndices = [sublo, subhi];
    let CoveredBySubRegs = true;
  }
}

class MOSRegClass<list<ValueType> regTypes, int alignment, dag regList>
     : RegisterClass<"MOS", regTypes, alignment, regList>;

def GPR : MOSRegClass<[i8], 8, (add A, X, Y)>;

def Flag : MOSRegClass<[i1], 8, (add C, N, Z)> {
  let isAllocatable = false;
}

let isPressureFineGrained = true in {
  def Ac : MOSRegClass<[i8], 8, (add A)>;
  def Yc : MOSRegClass<[i8], 8, (add Y)>;
  def XY : MOSRegClass<[i8], 8, (add X, Y)>;

  def Cc : MOSRegClass<[i1], 8, (add C)>;
}

def ZP : MOSRegClass<[i8], 8, (add SPlo, SPhi, (sequence "ZP_%u", 0, 253))>;
def ZP_PTR : MOSRegClass<[i16], 8, (add SP, (sequence "ZP_PTR_%u", 0, 126))>;

def Stack : MOSRegClass<[i8, i16], 8, (add Static, S, X, ZP_PTR)> {
  let isAllocatable = false;
}

def AZP : MOSRegClass<[i8], 8, (add A, ZP)>;

def Anyi8 : MOSRegClass<[i8], 8, (add GPR, ZP)>;
def Anyi1 : MOSRegClass<[i1], 8, (add GPR, ZP, Cc)>;

def Anyc : MOSRegClass<[i8, i16], 8, (add Anyi8, ZP_PTR)> {
  let isAllocatable = false;
}

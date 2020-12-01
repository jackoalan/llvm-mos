let Namespace = "MOS6502" in {

def sublo : SubRegIndex<8>;
def subhi : SubRegIndex<8>;

}

class MOS6502Reg<string n> : Register<n> {
  let Namespace = "MOS6502";
}

// Special registers used as the frame and stack pointer.
def SP : MOS6502Reg<"SP">;

def A : MOS6502Reg<"A">;
def X : MOS6502Reg<"X">;
def Y : MOS6502Reg<"Y">;

def P : MOS6502Reg<"P">;

def ZP_0 : MOS6502Reg<"ZP_0">;
def ZP_1 : MOS6502Reg<"ZP_1">;

def ZP_PTR_0 : MOS6502Reg<"ZP_PTR_0"> {
  let SubRegs = [ZP_0, ZP_1];
  let SubRegIndices = [sublo, subhi];
}

class MOS6502RegClass<list<ValueType> regTypes, int alignment, dag regList>
     : RegisterClass<"MOS6502", regTypes, alignment, regList>;

def GPR : MOS6502RegClass<[i8], 8, (add A, X, Y)>;

def Ac : MOS6502RegClass<[i8], 8, (add A)>;
def XY : MOS6502RegClass<[i8], 8, (add X, Y)>;

def ZP : MOS6502RegClass<[i8], 8, (add ZP_0, ZP_1)>;
def ZP_PTR : MOS6502RegClass<[i16], 8, (add ZP_PTR_0)>;

def Anyc : MOS6502RegClass<[i8], 8, (add GPR, ZP_PTR)>;

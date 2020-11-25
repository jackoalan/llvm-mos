class MOS6502Reg<string n> : Register<n> {
  let Namespace = "MOS6502";
}

// Special registers used as the frame and stack pointer.
def SP : MOS6502Reg<"SP">;

def A : MOS6502Reg<"A">;
def X : MOS6502Reg<"X">;
def Y : MOS6502Reg<"Y">;

def P : MOS6502Reg<"P">;

def ZP_PTR_0 : MOS6502Reg<"ZP_PTR_0">;

class MOS6502RegClass<list<ValueType> regTypes, int alignment, dag regList>
     : RegisterClass<"MOS6502", regTypes, alignment, regList>;

def GPR : MOS6502RegClass<[i8], 8, (add A, X, Y)>;

def ZP_PTR : MOS6502RegClass<[i16], 8, (add ZP_PTR_0)>;

def XY : MOS6502RegClass<[i8], 8, (add X, Y)> {
  let isAllocatable = false;
}

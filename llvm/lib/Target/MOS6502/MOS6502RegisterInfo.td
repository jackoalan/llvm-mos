class MOS6502Reg<string n> : Register<n> {
  let Namespace = "MOS6502";
}

// Special registers used as the frame and stack pointer.
def SP : MOS6502Reg<"%SP">;

def A : MOS6502Reg<"%A">;
def X : MOS6502Reg<"%X">;

class MOS6502RegClass<list<ValueType> regTypes, int alignment, dag regList>
     : RegisterClass<"MOS6502", regTypes, alignment, regList>;


def GPR : MOS6502RegClass<[i8], 8, (add A, X)>;
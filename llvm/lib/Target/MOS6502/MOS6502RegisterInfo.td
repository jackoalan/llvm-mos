class MOS6502Reg<string n> : Register<n> {
  let Namespace = "MOS6502";
}

class MOS6502RegClass<list<ValueType> regTypes, int alignment, dag regList>
     : RegisterClass<"MOS6502", regTypes, alignment, regList>;

// Special registers used as the frame and stack pointer.
def FP : MOS6502Reg<"%FP">;
def SP : MOS6502Reg<"%SP">;

// The register allocation framework requires register classes have at least
// one register, even though no physical registers are needed.
def I8_0 : MOS6502Reg<"%i8.0">;
def I16_0 : MOS6502Reg<"%i16.0">;

def P : MOS6502Reg<"%p">;

def I8 : MOS6502RegClass<[i8], 8, (add I8_0)>;
def I16 : MOS6502RegClass<[i16], 8, (add FP, SP, I16_0)>;
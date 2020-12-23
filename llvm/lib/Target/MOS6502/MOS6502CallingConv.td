def CC_MOS6502 : CallingConv<[CCIfType<[i8], CCAssignToReg<[A, X, Y]>>]> {
  let Entry = 1;
}

def RetCC_MOS6502 : CallingConv<[CCIfType<[i8], CCAssignToReg<[A, X, Y]>>]> {
  let Entry = 1;
}

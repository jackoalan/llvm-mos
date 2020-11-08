def RetCC_MOS6502 : CallingConv<[CCIfType<[i8], CCAssignToReg<[A, X]>>]> {
  let Entry = 1;
}

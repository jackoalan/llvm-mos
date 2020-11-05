def CC_MOS6502 : CallingConv<[CCIfType<[i8], CCAssignToStack<0, 0>>]> {
  let Entry = 1;
}

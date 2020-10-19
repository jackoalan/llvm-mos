def ADC : Instruction {
  let Namespace = "MOS6502";
  dag InOperandList = (ins I8:$rs1, I8:$rs2);
  dag OutOperandList = (outs I8:$rd);
  let Uses = [P];
  let Defs = [P];
  let mayLoad = 0;
  let mayStore = 0;
  let hasSideEffects = 0;
}
def RETURN : Instruction {
  let Namespace = "MOS6502";
  dag InOperandList = (ins variable_ops);
  dag OutOperandList = (outs);
  let Uses = [];
  let Defs = [];

  let hasCtrlDep = 1;
  let hasSideEffects = 0;
  let isPseudo = 1;
  let isReturn = 1;
  let isTerminator = 1;
  let mayLoad = 0;
  let mayStore = 0;
}
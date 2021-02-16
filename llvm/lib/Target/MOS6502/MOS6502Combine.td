def MOS6502CombinerHelper: GICombinerHelper<
  "MOS6502GenCombinerHelper", [all_combines]> {
  let DisableRuleOption = "mos6502combiner-disable-rule";
  let StateClass = "MOS6502CombinerHelperState";
  let AdditionalArguments = [];
}

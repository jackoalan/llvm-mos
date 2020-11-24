def MOS6502PreLegalizerCombinerHelper: GICombinerHelper<
  "MOS6502GenPreLegalizerCombinerHelper", [all_combines]> {
  let DisableRuleOption = "mos6502prelegalizercombiner-disable-rule";
  let StateClass = "MOS6502PreLegalizerCombinerHelperState";
  let AdditionalArguments = [];
}

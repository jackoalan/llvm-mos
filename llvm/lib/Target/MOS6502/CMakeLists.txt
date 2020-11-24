add_llvm_component_group(MOS6502)

set(LLVM_TARGET_DEFINITIONS MOS6502.td)

tablegen(LLVM MOS6502GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM MOS6502GenCallingConv.inc -gen-callingconv)
tablegen(LLVM MOS6502GenGlobalISel.inc -gen-global-isel)
tablegen(LLVM MOS6502GenPreLegalizeGICombiner.inc -gen-global-isel-combiner
  -combiners="MOS6502PreLegalizerCombinerHelper")
tablegen(LLVM MOS6502GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM MOS6502GenRegisterBank.inc -gen-register-bank)
tablegen(LLVM MOS6502GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM MOS6502GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(MOS6502CommonTableGen)

add_llvm_target(MOS6502CodeGen
  MOS6502AsmPrinter.cpp
  MOS6502CallingConv.cpp
  MOS6502CallLowering.cpp
  MOS6502FrameLowering.cpp
  MOS6502InstructionSelector.cpp
  MOS6502InstrInfo.cpp
  MOS6502ISelLowering.cpp
  MOS6502LegalizerInfo.cpp
  MOS6502MCInstLower.cpp
  MOS6502PreLegalizerCombiner.cpp
  MOS6502RegisterBankInfo.cpp
  MOS6502RegisterInfo.cpp
  MOS6502Subtarget.cpp
  MOS6502TargetMachine.cpp
  MOS6502TargetObjectFile.cpp

  ADD_TO_COMPONENT
  MOS6502
)

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)

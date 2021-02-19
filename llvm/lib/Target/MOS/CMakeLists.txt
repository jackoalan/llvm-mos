add_llvm_component_group(MOS)

set(LLVM_TARGET_DEFINITIONS MOS.td)

tablegen(LLVM MOSGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM MOSGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM MOSGenCallingConv.inc -gen-callingconv)
tablegen(LLVM MOSGenGlobalISel.inc -gen-global-isel)
tablegen(LLVM MOSGenGICombiner.inc -gen-global-isel-combiner
  -combiners="MOSCombinerHelper")
tablegen(LLVM MOSGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM MOSGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM MOSGenRegisterBank.inc -gen-register-bank)
tablegen(LLVM MOSGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM MOSGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(MOSCommonTableGen)

add_llvm_target(MOSCodeGen
  MOSAsmPrinter.cpp
  MOSCallingConv.cpp
  MOSCallLowering.cpp
  MOSFrameLowering.cpp
  MOSInstructionSelector.cpp
  MOSInstrInfo.cpp
  MOSIndexIVPass.cpp
  MOSISelLowering.cpp
  MOSLegalizerInfo.cpp
  MOSMachineFunctionInfo.cpp
  MOSMachineScheduler.cpp
  MOSMCInstLower.cpp
  MOSNoRecurse.cpp
  MOSCombiner.cpp
  MOSPreRegAlloc.cpp
  MOSRegisterBankInfo.cpp
  MOSRegisterInfo.cpp
  MOSStaticStackAlloc.cpp
  MOSSubtarget.cpp
  MOSTargetMachine.cpp
  MOSTargetObjectFile.cpp
  
  DEPENDS
  intrinsics_gen

  LINK_COMPONENTS
  IPO

  ADD_TO_COMPONENT
  MOS
)

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)

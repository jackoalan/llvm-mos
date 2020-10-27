set(LLVM_TARGET_DEFINITIONS MOS6502.td)

tablegen(LLVM MOS6502GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM MOS6502GenRegisterBank.inc -gen-register-bank)
tablegen(LLVM MOS6502GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM MOS6502GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(MOS6502CommonTableGen)

add_llvm_target(MOS6502CodeGen
  MOS6502CallLowering.cpp
  MOS6502FrameLowering.cpp
  MOS6502InstructionSelector.cpp
  MOS6502InstrInfo.cpp
  MOS6502LegalizerInfo.cpp
  MOS6502RegisterBankInfo.cpp
  MOS6502RegisterInfo.cpp
  MOS6502Subtarget.cpp
  MOS6502TargetMachine.cpp
)

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)

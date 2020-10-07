set(LLVM_TARGET_DEFINITIONS MOS6502.td)

add_llvm_target(MOS6502CodeGen
  MOS6502TargetMachine.cpp)

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)

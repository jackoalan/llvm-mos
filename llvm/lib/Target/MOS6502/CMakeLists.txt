set(LLVM_TARGET_DEFINITIONS MOS6502.td)

tablegen(LLVM MOS6502GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(MOS6502CommonTableGen)

add_llvm_target(MOS6502CodeGen
  MOS6502Subtarget.cpp
  MOS6502TargetMachine.cpp)

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)

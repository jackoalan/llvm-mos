#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502INSTRUCTIONSELECTOR_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502INSTRUCTIONSELECTOR_H

#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"

namespace llvm {

class MOS6502InstructionSelector : public InstructionSelector {
 public:
  bool select(MachineInstr &I) override;
};

}  // namespace llvm

#endif   // not LLVM_LIB_TARGET_MOS6502_MOS6502INSTRUCTIONSELECTOR_H

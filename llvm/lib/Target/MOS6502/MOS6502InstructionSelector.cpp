#include "MOS6502InstructionSelector.h"

using namespace llvm;

bool MOS6502InstructionSelector::select(MachineInstr &I) {
  return false;
}

void MOS6502InstructionSelector::setupGeneratedPerFunctionState(MachineFunction &MF) {
  // Disable assertion in base class; we're not using SelectionDAG TableGen.
}

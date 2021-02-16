#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502MACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502MACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

class MOS6502FunctionInfo : public MachineFunctionInfo {
  int VarArgsStackIndex;

public:
  MOS6502FunctionInfo(MachineFunction& MF) {}

  int getVarArgsStackIndex() const { return VarArgsStackIndex; }
  void setVarArgsStackIndex(int Index) { VarArgsStackIndex = Index; }
};

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502MACHINEFUNCTIONINFO_H
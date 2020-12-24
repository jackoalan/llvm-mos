#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502TARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502TARGETTRANSFORMINFO_H

#include "MOS6502TargetMachine.h"
#include "llvm/CodeGen/BasicTTIImpl.h"

namespace llvm {

class MOS6502TTIImpl : public BasicTTIImplBase<MOS6502TTIImpl> {
  using BaseT = BasicTTIImplBase<MOS6502TTIImpl>;

  friend BaseT;

  const MOS6502Subtarget *ST;
  const MOS6502TargetLowering *TLI;

  const MOS6502Subtarget *getST() const { return ST; }
  const MOS6502TargetLowering *getTLI() const { return TLI; }

public:
  explicit MOS6502TTIImpl(const MOS6502TargetMachine *TM, const Function &F)
      : BaseT(TM, F.getParent()->getDataLayout()), ST(TM->getSubtargetImpl(F)),
        TLI(ST->getTargetLowering()) {}

  // All div, rem, and divrem ops are libcalls, so any possible combination
  // exists.
  bool hasDivRemOp(Type *DataType, bool IsSigned) { return true; }
};

} // end namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502TARGETTRANSFORMINFO_H

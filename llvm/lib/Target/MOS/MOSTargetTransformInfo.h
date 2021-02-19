#ifndef LLVM_LIB_TARGET_MOS_MOSTARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_MOS_MOSTARGETTRANSFORMINFO_H

#include "MOSTargetMachine.h"
#include "llvm/CodeGen/BasicTTIImpl.h"

namespace llvm {

class MOSTTIImpl : public BasicTTIImplBase<MOSTTIImpl> {
  using BaseT = BasicTTIImplBase<MOSTTIImpl>;

  friend BaseT;

  const MOSSubtarget *ST;
  const MOSTargetLowering *TLI;

  const MOSSubtarget *getST() const { return ST; }
  const MOSTargetLowering *getTLI() const { return TLI; }

public:
  explicit MOSTTIImpl(const MOSTargetMachine *TM, const Function &F)
      : BaseT(TM, F.getParent()->getDataLayout()), ST(TM->getSubtargetImpl(F)),
        TLI(ST->getTargetLowering()) {}

  // All div, rem, and divrem ops are libcalls, so any possible combination
  // exists.
  bool hasDivRemOp(Type *DataType, bool IsSigned) { return true; }
};

} // end namespace llvm

#endif // not LLVM_LIB_TARGET_MOS_MOSTARGETTRANSFORMINFO_H

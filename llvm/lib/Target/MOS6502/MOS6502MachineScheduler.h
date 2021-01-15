#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502_MACHINESCHEDULER_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502_MACHINESCHEDULER_H

#include "llvm/CodeGen/MachineScheduler.h"

namespace llvm {

class MOS6502SchedStrategy : public GenericScheduler {
public:
  MOS6502SchedStrategy(const MachineSchedContext *C);

  void tryCandidate(SchedCandidate &Cand, SchedCandidate &TryCand,
                    SchedBoundary *Zone) const override;
};

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502_MACHINESCHEDULER_H

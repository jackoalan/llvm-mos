#ifndef LLVM_LIB_TARGET_MOS_MOSMACHINELEGALIZER_H
#define LLVM_LIB_TARGET_MOS_MOSMACHINELEGALIZER_H

#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

namespace llvm {

class MOSLegalizerInfo : public LegalizerInfo {
public:
  MOSLegalizerInfo();

  bool legalizeCustom(LegalizerHelper &Helper, MachineInstr &MI) const override;
  bool legalizeFrameIndex(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeUAddSubO(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeLoad(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizePtrAdd(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeShl(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeStore(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeVAArg(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeVAStart(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
};

} // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS_MOSMACHINELEGALIZER_H

#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502MACHINELEGALIZER_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502MACHINELEGALIZER_H

#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

namespace llvm {

class MOS6502LegalizerInfo : public LegalizerInfo {
public:
  MOS6502LegalizerInfo();

  bool legalizeCustom(LegalizerHelper &Helper, MachineInstr &MI) const override;
  bool legalizeFrameIndex(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeUAddO(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeLoad(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizePtrAdd(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeShl(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeStore(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeVAArg(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
  bool legalizeVAStart(LegalizerHelper &Helper, MachineRegisterInfo& MRI, MachineInstr &MI) const;
};

} // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MOS6502MACHINELEGALIZER_H

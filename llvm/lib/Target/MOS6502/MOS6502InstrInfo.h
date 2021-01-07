#ifndef LLVM_LIB_TARGET_MOS6502_MOS6502INSTRINFO_H
#define LLVM_LIB_TARGET_MOS6502_MOS6502INSTRINFO_H

#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "MOS6502GenInstrInfo.inc"

namespace llvm {

class MOS6502InstrInfo : public MOS6502GenInstrInfo {
public:
  bool isReallyTriviallyReMaterializable(const MachineInstr &MI,
                                         AAResults *AA) const override;

  MachineInstr *commuteInstructionImpl(MachineInstr &MI, bool NewMI,
                                       unsigned OpIdx1,
                                       unsigned OpIdx2) const override;

  void reMaterialize(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                     Register DestReg, unsigned SubIdx,
                     const MachineInstr &Orig,
                     const TargetRegisterInfo &TRI) const override;

  unsigned getInstSizeInBytes(const MachineInstr &MI) const override;

  bool isBranchOffsetInRange(unsigned BranchOpc,
                             int64_t BrOffset) const override;

  MachineBasicBlock *getBranchDestBlock(const MachineInstr &MI) const override;

  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify = false) const override;

  unsigned removeBranch(MachineBasicBlock &MBB,
                        int *BytesRemoved = nullptr) const override;

  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &DL,
                        int *BytesAdded = nullptr) const override;

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   const DebugLoc &DL, MCRegister DestReg, MCRegister SrcReg,
                   bool KillSrc) const override;

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, Register SrcReg,
                           bool isKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI) const override;

  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MI, Register DestReg,
                            int FrameIndex, const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI) const override;

  bool expandPostRAPseudo(MachineInstr &MI) const override;

  bool
  reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;

  std::pair<unsigned, unsigned>
  decomposeMachineOperandsTargetFlags(unsigned TF) const override;

  ArrayRef<std::pair<unsigned, const char *>>
  getSerializableDirectMachineOperandTargetFlags() const override;

private:
  // Various locations throughout codegen emit pseudo-instructions with very few
  // implicit defs. This is required whenever LLVM codegen cannot handle
  // emitting arbitrary physreg uses, for example, during COPY or
  // saveRegToStack.
  //
  // Often, such pseudos cannot be expanded without producing significantly more
  // side effects than the pseudo allows. This function takes a Builder pointing
  // to a pseudo, then calls ExpandFn to expand the pseudo. Then, the physreg
  // defs of the expanded instructions are measured, and save and restore code
  // is emitted to ensure that the pseudo expansion region only modifies defs of
  // the pseudo.
  //
  // ExpandFn must insert a contiguous range of instructions before the pseudo.
  // Afterwards, the Builder must point to the location after the inserted
  // range.
  void preserveAroundPseudoExpansion(MachineIRBuilder &Builder,
                                     std::function<void()> ExpandFn) const;

  void copyPhysRegNoPreserve(MachineIRBuilder &Builder, MCRegister DestReg,
                             MCRegister SrcReg) const;

  bool expandPostRAPseudoNoPreserve(MachineIRBuilder &Builder) const;
};

namespace MOS6502 {

enum TOF {
  MO_NO_FLAGS = 0,
  MO_LO,
  MO_HI,
};

} // namespace MOS6502

} // namespace llvm

#endif // not LLVM_LIB_TARGET_MOS6502_MOS6502INSTRINFO_H

#include "MOS6502FrameLowering.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"

#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

MOS6502FrameLowering::MOS6502FrameLowering()
    : TargetFrameLowering(StackGrowsDown, /*StackAlignment=*/Align(1),
                          /*LocalAreaOffset=*/0) {}

void MOS6502FrameLowering::emitPrologue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  if (MFI.getStackSize() > 4)
    report_fatal_error("Only small (<4 bytes) HS uses are supported.");

  auto MI = MBB.begin();
  // Value of S at MI, relative to its value on entry.
  int SPOffset = 0;

  // Push undef to the stack until SPOffset has increased to NewOffset.
  const auto PushUntil = [&](int NewOffset) {
    MachineIRBuilder Builder(MBB, MI);
    // It doesn't matter what is pushed, just that the stack pointer is
    // increased. This is still at least as efficient as increasing S using X
    // and/or A.
    for (; SPOffset < NewOffset; ++SPOffset) {
      auto Push = Builder.buildInstr(MOS6502::PHA);
      Push->implicit_operands().begin()->setIsUndef();
    }
  };

  // Defer pushing to the stack as long as possible. Hopefully, this will allow
  // folding SThs together with the PHA that increase the stack pointer.
  for (; MI != MBB.end(); ++MI) {
    // For now, this is basic-block only.
    if (MI->isTerminator())
      break;

    // Loads are relative to the fully-offset SP, so if we see one, we can't
    // defer any longer.
    if (MI->getOpcode() == MOS6502::LDhs)
      break;

    // Only SThs and LDhs affect the stack.
    if (MI->getOpcode() != MOS6502::SThs)
      continue;

    // We only can elide PHA, so the value must be in A.
    if (MI->getOperand(0).getReg() != MOS6502::A)
      break;

    // If the SThs is to a higher SP, we can increase SP up to this point,
    // allowing us to fold the store.
    if (SPOffset < MI->getOperand(1).getIndex())
      PushUntil(MI->getOperand(1).getIndex());
    // If the SThs is to a lower SP, it cannot be folded, and is relative to the
    // fully-offset S, so we're done.
    else if (SPOffset > MI->getOperand(1).getIndex())
      break;

    // The SThs == SPOffset, so it can folded together with a PHA.
    assert(MI->getOperand(1).getIndex() == SPOffset);
    MI->RemoveOperand(1);
    MI->RemoveOperand(0);
    MI->setDesc(MF.getSubtarget().getInstrInfo()->get(MOS6502::PHA));
    // This PHA actually does use A; it's not undef.
    MI->addImplicitDefUseOperands(MF);
    ++SPOffset;
  }

  // Push any remaining bytes necessary to fully increase S.
  PushUntil(MFI.getStackSize());
}

void MOS6502FrameLowering::emitEpilogue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (!MFI.getStackSize())
    return;

  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();

  auto MI = MBB.getFirstTerminator();
  if (MI == MBB.end())
    --MI;

  MachineIRBuilder Builder(MBB, MI);
  bool AMaybeLive = MBB.computeRegisterLiveness(&TRI, MOS6502::A, MI) !=
                    MachineBasicBlock::LQR_Dead;
  assert(MBB.computeRegisterLiveness(&TRI, MOS6502::NZ, MI) !=
         MachineBasicBlock::LQR_Live);

  if (AMaybeLive)
    Builder.buildInstr(MOS6502::STzpr).addDef(MOS6502::ZP_0).addUse(MOS6502::A);

  // It doesn't matter what is pushed, just that the stack pointer is increased.
  // This is still at least as efficient as increasing S using X and/or A.
  for (uint64_t I = 0; I < MFI.getStackSize(); ++I)
    Builder.buildInstr(MOS6502::PLA);

  if (AMaybeLive)
    Builder.buildInstr(MOS6502::LDzpr).addDef(MOS6502::A).addUse(MOS6502::ZP_0);
}

bool MOS6502FrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.isFrameAddressTaken() || MFI.hasVarSizedObjects())
    report_fatal_error("Frame pointer not yet supported.");
  return false;
}

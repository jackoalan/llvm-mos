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

#define DEBUG_TYPE "mos6502-framelowering"

using namespace llvm;

MOS6502FrameLowering::MOS6502FrameLowering()
    : TargetFrameLowering(StackGrowsDown, /*StackAlignment=*/Align(1),
                          /*LocalAreaOffset=*/0) {}

void MOS6502FrameLowering::emitPrologue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  if (MFI.getStackSize() > 4)
    report_fatal_error("Only small (<4 bytes) HS uses are supported.");

  LLVM_DEBUG(dbgs() << "Emitting Prologue:\n");

  auto MI = MBB.begin();
  // Value of S at MI, relative to its value on entry.
  int S = 0;

  // Push undef to the stack until S has decreased to NewS.
  const auto PushUntil = [&](int NewS) {
    LLVM_DEBUG(dbgs() << "Pushing until S=" << NewS << "\n");

    MachineIRBuilder Builder(MBB, MI);
    // It doesn't matter what is pushed, just that the stack pointer is
    // decreased. This is still at least as efficient as increasing S using X
    // and/or A.
    for (; S > NewS; --S) {
      auto Push = Builder.buildInstr(MOS6502::PHA);
      Push->implicit_operands().begin()->setIsUndef();
    }
  };

  // Defer pushing to the stack as long as possible. Hopefully, this will allow
  // folding SThs together with the PHA that decreases the stack pointer.
  for (; MI != MBB.end(); ++MI) {
    // For now, this is basic-block only.
    if (MI->isTerminator())
      break;

    // Loads are relative to the fully-offset S, so if we see one, we can't
    // defer any longer.
    if (MI->getOpcode() == MOS6502::LDhs)
      break;

    // Only SThs and LDhs affect the stack.
    if (MI->getOpcode() != MOS6502::SThs)
      continue;

    // We only can elide PHA, so the value must be in A.
    if (MI->getOperand(0).getReg() != MOS6502::A)
      break;

    // S begins (S = 0) pointing at Offset -1, not Offset 0, since it points to
    // the byte after the top of stack. Increase offsets by one byte to
    // compensate.
    int64_t Offset = MFI.getObjectOffset(MI->getOperand(1).getIndex()) + 1;
    LLVM_DEBUG(dbgs() << "S: " << S << "\n");
    LLVM_DEBUG(dbgs() << "SThs Offset: " << Offset << "\n");

    // If the SThs is to a lower S, we can decrease S up to this point,
    // allowing us to fold the store.
    if (S > Offset)
      PushUntil(Offset);
    // If the SThs is to a higher S, it cannot be folded. It's relative to the
    // fully-offset S, so we're done.
    else if (S < Offset)
      break;

    // The SThs == S, so it can folded together with a PHA.
    assert(Offset == S);
    MI->RemoveOperand(1);
    MI->RemoveOperand(0);
    MI->setDesc(MF.getSubtarget().getInstrInfo()->get(MOS6502::PHA));
    // This PHA actually does use A; it's not undef.
    MI->addImplicitDefUseOperands(MF);
    --S;
  }

  // Push any remaining bytes necessary to fully decrease S.
  PushUntil(-MFI.getStackSize());
}

void MOS6502FrameLowering::emitEpilogue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (!MFI.getStackSize())
    return;

  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();

  auto MI = MBB.getFirstTerminator();
  MachineIRBuilder Builder(MBB, MI);

  LLVM_DEBUG(dbgs() << "Emitting Epilogue:\n");

  // Value of S at MI, relative to its value on exit.
  int S = 0;

  // Emit pull instructions before MI as necessary to decrease S to
  // NewS.
  const auto PullUntil = [&](int NewS) {
    if (S <= NewS)
      return;

    if (MBB.computeRegisterLiveness(&TRI, MOS6502::NZ, MI) !=
        MachineBasicBlock::LQR_Dead)
      report_fatal_error("Cannot yet save NZ.");

    bool AMaybeLive = MBB.computeRegisterLiveness(&TRI, MOS6502::A, MI) !=
                      MachineBasicBlock::LQR_Dead;

    MachineIRBuilder Builder(MBB, MI);

    LLVM_DEBUG(dbgs() << "Pulling until S=" << NewS << "\n");

    if (AMaybeLive)
      Builder.buildInstr(MOS6502::STzpr)
          .addDef(MOS6502::ZP_0)
          .addUse(MOS6502::A);

    for (; S > NewS; --S) {
      auto Pull = Builder.buildInstr(MOS6502::PLA);
      Pull->implicit_operands().begin()->setIsDead();
    }

    if (AMaybeLive)
      Builder.buildInstr(MOS6502::LDzpr)
          .addDef(MOS6502::A)
          .addUse(MOS6502::ZP_0);
  };

  // Defer pulling from the stack to as early as possible. Hopefully, this will
  // allow folding LDhs together with the PLA that decrease the stack pointer.
  for (; MI != MBB.begin(); --MI) {
    auto PrevMI = std::prev(MI);
    // For now, this operates on one basic block only.
    if (PrevMI == MBB.begin()) {
      --MI;
      break;
    }

    // Stores are relative to the fully-offset S, so if we see one, we can't
    // defer any longer.
    if (PrevMI->getOpcode() == MOS6502::SThs)
      break;

    // Prologues have already been emitted, so if we run into a PHA, bail.
    if (PrevMI->getOpcode() == MOS6502::PHA)
      break;

    // Only SThs and LDhs affect the stack.
    if (PrevMI->getOpcode() != MOS6502::LDhs)
      continue;

    // We only can elide PLA, so the destination must be A.
    if (PrevMI->getOperand(0).getReg() != MOS6502::A)
      break;

    // S begins (S = 0) pointing at Offset -1, not Offset 0, since it points to
    // the byte after the top of stack. Increase offsets by one byte to
    // compensate.
    int64_t Offset = MFI.getObjectOffset(PrevMI->getOperand(1).getIndex()) + 1;
    LLVM_DEBUG(dbgs() << "S: " << S << "\n");
    LLVM_DEBUG(dbgs() << "LDhs Offset: " << Offset << "\n");

    // If the LDhs is to a lower S, we can decrease S by emitting PLA to
    // reach that S, allowing the load to be folded with a PLA.
    if (S > Offset)
      PullUntil(Offset);
    // If the LDhs is to a higher S, it cannot be folded and is relative to the
    // fully-offset S, so we're done.
    else if (S < Offset)
      break;

    // The LDhs == S, so it can folded together with a PLA.
    assert(Offset == S);
    PrevMI->RemoveOperand(1);
    PrevMI->RemoveOperand(0);
    PrevMI->setDesc(MF.getSubtarget().getInstrInfo()->get(MOS6502::PLA));
    // This PLA actually does set A to a meaningful value; it's not dead.
    PrevMI->addImplicitDefUseOperands(MF);
    --S;
  }

  // Pull any remaining bytes necessary to fully decrease S.
  PullUntil(-MFI.getStackSize());
}

bool MOS6502FrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.isFrameAddressTaken() || MFI.hasVarSizedObjects())
    report_fatal_error("Frame pointer not yet supported.");
  return false;
}

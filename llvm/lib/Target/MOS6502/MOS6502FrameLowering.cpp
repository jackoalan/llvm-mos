#include "MOS6502FrameLowering.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"

#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/ErrorHandling.h"

#define DEBUG_TYPE "mos6502-framelowering"

using namespace llvm;

MOS6502FrameLowering::MOS6502FrameLowering()
    : TargetFrameLowering(StackGrowsDown, /*StackAlignment=*/Align(1),
                          /*LocalAreaOffset=*/0) {}

void MOS6502FrameLowering::processFunctionBeforeFrameFinalized(
    MachineFunction &MF, RegScavenger *RS) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();

  // Targeted value of S at end of prologue, with S=0 on entry.
  int S = 0;

  // Extract out as many values as will fit onto the hard stack.
  for (int i = 0, e = MFI.getObjectIndexEnd(); i < e && S > -4; ++i) {
    int64_t Size = MFI.getObjectSize(i);
    if (S - Size < -4)
      continue;
    MFI.setStackID(i, TargetStackID::Hard);
    // Hard stack offsets are S-relative, and S already points at the first byte
    // of the object, since it's always one byte past the end of the stack.
    MFI.setObjectOffset(i, S);
    MFI.setStackSize(MFI.getStackSize() - Size);
    S -= Size;
  }
}

void MOS6502FrameLowering::emitPrologue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  auto MI = MBB.begin();

  // If soft stack is used, decrease the soft stack pointer SP.
  if (MFI.getStackSize()) {
    MachineIRBuilder Builder(MBB, MI);
    Builder.buildInstr(MOS6502::IncSP).addImm(-MFI.getStackSize());
  }

  uint64_t HsSize = hsSize(MFI);

  LLVM_DEBUG(dbgs() << "Emitting Prologue:\n");

  // Value of S at MI, relative to its value on entry.
  int S = 0;

  // Push undef to the stack until S has decreased to NewS.
  const auto PushUntil = [&](int NewS) {
    LLVM_DEBUG(dbgs() << "S:" << S << "\n");
    LLVM_DEBUG(dbgs() << "Pushing until S=" << NewS << "\n");

    MachineIRBuilder Builder(MBB, MI);
    // It doesn't matter what is pushed, just that the stack pointer is
    // decreased. This is still at least as efficient as increasing S using X
    // and/or A.
    for (; S > NewS; --S) {
      Builder.buildInstr(MOS6502::Pushhs).addUse(MOS6502::A, RegState::Undef);
    }
  };

  // Defer pushing to the stack as long as possible. Hopefully, this will allow
  // folding SThs together with the push that decreases the stack pointer.
  for (; MI != MBB.end(); ++MI) {
    // For now, this is basic-block only.
    if (MI->isTerminator())
      break;

    bool IsLoad = false;
    unsigned PushOpcode;
    switch (MI->getOpcode()) {
    default:
      continue;
    case MOS6502::LDhs:
    case MOS6502::LDPtrhs:
      IsLoad = true;
      break;
    case MOS6502::SThs:
      PushOpcode = MOS6502::Pushhs;
      break;
    case MOS6502::STPtrhs:
      PushOpcode = MOS6502::PushPtrhs;
      break;
    }

    // Loads are relative to the fully-offset S, so if we see one, we can't
    // defer any longer.
    if (IsLoad)
      break;

    unsigned Idx = MI->getOperand(1).getIndex();
    int64_t Offset = MFI.getObjectOffset(Idx);
    LLVM_DEBUG(dbgs() << "S: " << S << "\n");
    LLVM_DEBUG(dbgs() << "Index Offset: " << Offset << "\n");

    // If the SThs is to a lower S, we can decrease S up to this point,
    // allowing us to fold the store.
    if (S > Offset)
      PushUntil(Offset);
    // If the SThs is to a higher S, it cannot be folded. It's relative to the
    // fully-offset S, so we're done.
    else if (S < Offset)
      break;

    // The SThs == S, so it can folded together with a Push.
    assert(Offset == S);
    MI->RemoveOperand(1);
    MI->setDesc(MF.getSubtarget().getInstrInfo()->get(PushOpcode));
    MI->addImplicitDefUseOperands(MF);
    S -= MFI.getObjectSize(Idx);
  }

  // Push any remaining bytes necessary to fully decrease S.
  PushUntil(-HsSize);
}

void MOS6502FrameLowering::emitEpilogue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  auto MI = MBB.getFirstTerminator();

  // If soft stack is used, increase the soft stack pointer SP.
  if (MFI.getStackSize()) {
    MachineIRBuilder Builder(MBB, MI);
    Builder.buildInstr(MOS6502::IncSP).addImm(MFI.getStackSize());
  }

  uint64_t HsSize = hsSize(MFI);

  LLVM_DEBUG(dbgs() << "Emitting Epilogue:\n");

  // Value of S at MI, relative to its value on exit.
  int S = 0;

  // Emit pull instructions before MI as necessary to decrease S to
  // NewS.
  const auto PullUntil = [&](int NewS) {
    MachineIRBuilder Builder(MBB, MI);

    LLVM_DEBUG(dbgs() << "S:" << S << "\n");
    LLVM_DEBUG(dbgs() << "Pulling until S=" << NewS << "\n");

    for (; S > NewS; --S) {
      Builder.buildInstr(MOS6502::Pullhs).addDef(MOS6502::A, RegState::Dead);
    }
  };

  // Defer pulling from the stack to as early as possible. Hopefully, this will
  // allow folding LDhs together with the Pull that decreases the stack pointer.
  for (; MI != MBB.begin(); --MI) {
    auto PrevMI = std::prev(MI);

    bool Break = false;
    unsigned PullOpcode;
    switch (PrevMI->getOpcode()) {
    default:
      continue;
    case MOS6502::SThs:
    case MOS6502::STPtrhs:
    case MOS6502::Pushhs:
    case MOS6502::PushPtrhs:
      Break = true;
      break;
    case MOS6502::LDhs:
      PullOpcode = MOS6502::Pullhs;
      break;
    case MOS6502::LDPtrhs:
      PullOpcode = MOS6502::PullPtrhs;
      break;
    }
    if (Break)
      break;

    unsigned Idx = PrevMI->getOperand(1).getIndex();
    int64_t Offset = MFI.getObjectOffset(Idx);
    LLVM_DEBUG(dbgs() << "S: " << S << "\n");
    LLVM_DEBUG(dbgs() << "Index Offset: " << Offset << "\n");

    // If the LDhs is to a lower S, we can decrease S by emitting pulls to reach
    // that S, allowing the load to be folded with a pull.
    if (S > Offset)
      PullUntil(Offset);
    // If the LDhs is to a higher S, it cannot be folded and is relative to the
    // fully-offset S, so we're done.
    else if (S < Offset)
      break;

    // The LDhs == S, so it can folded together with a pull.
    assert(Offset == S);
    PrevMI->RemoveOperand(1);
    PrevMI->setDesc(MF.getSubtarget().getInstrInfo()->get(PullOpcode));
    PrevMI->addImplicitDefUseOperands(MF);
    S -= MFI.getObjectSize(Idx);
  }

  // Pull any remaining bytes necessary to fully decrease S.
  PullUntil(-HsSize);
}

bool MOS6502FrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.isFrameAddressTaken() || MFI.hasVarSizedObjects())
    report_fatal_error("Frame pointer not yet supported.");
  return false;
}

uint64_t MOS6502FrameLowering::hsSize(const MachineFrameInfo &MFI) const {
  uint64_t Size = 0;
  for (int i = 0, e = MFI.getObjectIndexEnd(); i < e; ++i)
    if (MFI.getStackID(i) == TargetStackID::Hard)
      Size += MFI.getObjectSize(i);
  return Size;
}

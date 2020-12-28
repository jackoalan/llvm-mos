#include "MOS6502FrameLowering.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"

#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
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

  MachineIRBuilder Builder(MBB, MBB.begin());
  // It doesn't matter what is pushed, just that the stack pointer is increased.
  // This is still at least as efficient as increasing S using X and/or A.
  for (uint64_t I = 0; I < MFI.getStackSize(); ++I)
    Builder.buildInstr(MOS6502::PHA);
}

void MOS6502FrameLowering::emitEpilogue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineIRBuilder Builder(MBB, MBB.getFirstTerminator());
  // It doesn't matter what is pushed, just that the stack pointer is increased.
  // This is still at least as efficient as increasing S using X and/or A.
  for (uint64_t I = 0; I < MFI.getStackSize(); ++I)
    Builder.buildInstr(MOS6502::PLA);
}

bool MOS6502FrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.isFrameAddressTaken() || MFI.hasVarSizedObjects())
    report_fatal_error("Frame pointer not yet supported.");
  return false;
}

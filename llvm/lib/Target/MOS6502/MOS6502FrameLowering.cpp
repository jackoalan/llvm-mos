#include "MOS6502FrameLowering.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"

using namespace llvm;

MOS6502FrameLowering::MOS6502FrameLowering()
  : TargetFrameLowering(StackGrowsDown, /*StackAlignment=*/Align(1),
                        /*LocalAreaOffset=*/0) {}

void MOS6502FrameLowering::emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const {
  assert(0 && "Not yet implemented.");
}
void MOS6502FrameLowering::emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const {
  assert(0 && "Not yet implemented.");
}
bool MOS6502FrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MFI.isFrameAddressTaken() || MFI.hasVarSizedObjects();
}

#include "MOS6502PreRegAlloc.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502.h"
#include "MOS6502RegisterInfo.h"

#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineOperand.h"

#define DEBUG_TYPE "mos6502-preregalloc"

using namespace llvm;

namespace {

struct MOS6502PreRegAlloc : public MachineFunctionPass {
  static char ID;

  MOS6502PreRegAlloc() : MachineFunctionPass(ID) {
    initializeMOS6502PreRegAllocPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
};

bool couldContainA(Register Reg, const MachineRegisterInfo &MRI) {
  if (Reg.isPhysical())
    return Reg == MOS6502::A;
  return MRI.getRegClass(Reg)->contains(MOS6502::A);
}

bool MOS6502PreRegAlloc::runOnMachineFunction(MachineFunction &MF) {
  MachineRegisterInfo &MRI = MF.getRegInfo();
  bool Changed = false;

  for (auto &MBB : MF) {
    for (auto &MI : MBB) {
      switch (MI.getOpcode()) {
      case MOS6502::ASL:
      case MOS6502::ROL: {
        Register Dst = MI.getOperand(0).getReg();
        Register Src = MI.getOperand(1).getReg();
        if (couldContainA(Dst, MRI) || !couldContainA(Src, MRI))
          continue;

        Register DstCopy = MRI.createVirtualRegister(&MOS6502::AZPRegClass);
        MI.getOperand(0).setReg(DstCopy);
        MachineIRBuilder Builder(MBB, std::next(MI.getIterator()));
        Builder.buildCopy(Dst, DstCopy);
        Changed = true;
        break;
      }
      }
    }
  }

  return Changed;
}

} // namespace

MachineFunctionPass *llvm::createMOS6502PreRegAlloc() {
  return new MOS6502PreRegAlloc;
}

char MOS6502PreRegAlloc::ID = 0;

INITIALIZE_PASS(MOS6502PreRegAlloc, DEBUG_TYPE,
                "Adjust instructions before register allocation.", false, false)

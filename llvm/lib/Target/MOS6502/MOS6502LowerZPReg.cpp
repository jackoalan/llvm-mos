#include "MOS6502LowerZPReg.h"

#include "MOS6502.h"
#include "MOS6502RegisterInfo.h"
#include "MOS6502Subtarget.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "llvm-c/Core.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "mos6502-lowerzpreg"

namespace llvm {

class MOS6502LowerZPReg : public MachineFunctionPass {
public:
  static char ID;

  MOS6502LowerZPReg();
  bool runOnMachineFunction(MachineFunction &MF) override;
};

} // namespace llvm

MOS6502LowerZPReg::MOS6502LowerZPReg() : MachineFunctionPass(ID) {
  initializeMOS6502LowerZPRegPass(*PassRegistry::getPassRegistry());
}

bool MOS6502LowerZPReg::runOnMachineFunction(MachineFunction &MF) {
  LLVM_DEBUG(dbgs() << "********** LOWERING MOS6502 ZP REGS **********\n");

  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  const MOS6502RegisterInfo &TRI = *MF.getSubtarget<MOS6502Subtarget>().getRegisterInfo();
  for (MachineBasicBlock &BB : MF) {
    for (MachineInstr &MI : BB) {
      if (!MI.isPseudo() || MI.isInlineAsm())
        continue;

      LLVM_DEBUG(dbgs() << "ZP Pseudo:\t" << MI);

      const auto LowerOp = [](MachineInstr &MI) {
        switch (MI.getOpcode()) {
        default:
          llvm_unreachable("Unhandled pseudoinstruction.");
        case MOS6502::ADCzpr:
          return MOS6502::ADCzp;
        case MOS6502::ASL:
          if (MI.getOperand(0).getReg() == MOS6502::A) {
            MI.RemoveOperand(1);
            MI.RemoveOperand(0);
            return MOS6502::ASLA;
          } else {
            assert(MOS6502::ZPRegClass.contains(MI.getOperand(0).getReg()));
            MI.RemoveOperand(0);
            return MOS6502::ASLzp;
          }
        case MOS6502::ROL:
          if (MI.getOperand(0).getReg() == MOS6502::A) {
            MI.RemoveOperand(1);
            MI.RemoveOperand(0);
            return MOS6502::ROLA;
          } else {
            assert(MOS6502::ZPRegClass.contains(MI.getOperand(0).getReg()));
            MI.RemoveOperand(0);
            return MOS6502::ROLzp;
          }
        case MOS6502::LDzpr:
          return MOS6502::LDzp;
        case MOS6502::LDAyindirr:
          return MOS6502::LDAyindir;
        case MOS6502::STzpr:
          return MOS6502::STzp;
        case MOS6502::STAyindirr:
          return MOS6502::STAyindir;
        }
      };

      MI.setDesc(TII.get(LowerOp(MI)));

      for (MachineOperand &MO : MI.operands()) {
        if (!MO.isReg() || MO.isImplicit())
          continue;
        Register Reg = MO.getReg();
        if (MOS6502::ZP_PTRRegClass.contains(Reg))
          Reg = TRI.getSubReg(Reg, MOS6502::sublo);
        if (!MOS6502::ZPRegClass.contains(Reg))
          continue;
        MO.ChangeToES(TRI.getZPSymbolName(Reg));
      }
      LLVM_DEBUG(dbgs() << "Replaced with:\t" << MI);
    }
  }

  return false;
}

char MOS6502LowerZPReg::ID = 0;

INITIALIZE_PASS(MOS6502LowerZPReg, DEBUG_TYPE,
                "Lower Zero Page 'registers' to memory locations", false, false)

MachineFunctionPass *llvm::createMOS6502LowerZPReg() { return new MOS6502LowerZPReg(); }

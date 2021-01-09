#include "MOS6502LowerZPReg.h"

#include "MOS6502.h"
#include "MOS6502RegisterInfo.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "llvm-c/Core.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "mos6502-lowerzpreg"

namespace llvm {

class MOS6502LowerZPReg : public ModulePass {
public:
  static char ID;

  MOS6502LowerZPReg();
  bool runOnModule(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};

} // namespace llvm

std::string GetZPName(MCRegister R, const MCRegisterInfo &MRI) {
  assert(MOS6502::ZPRegClass.contains(R) ||
         MOS6502::ZP_PTRRegClass.contains(R));
  return (Twine("_") + MRI.getName(R)).str();
}

MOS6502LowerZPReg::MOS6502LowerZPReg() : ModulePass(ID) {
  initializeMOS6502LowerZPRegPass(*PassRegistry::getPassRegistry());
}

bool MOS6502LowerZPReg::runOnModule(Module &M) {
  bool UsesSP = false;
  MCRegister HighestZP;
  MCRegister HighestZPPtr;

  LLVM_DEBUG(dbgs() << "********** LOWERING MOS6502 ZP REGS **********\n");

  MachineModuleInfo &MMI = getAnalysis<MachineModuleInfoWrapperPass>().getMMI();
  const MCRegisterInfo &MRI = *MMI.getTarget().getMCRegisterInfo();

  for (Function &F : M.functions()) {
    MachineFunction &MF = MMI.getOrCreateMachineFunction(F);
    for (MachineBasicBlock &BB : MF) {
      for (MachineInstr &I : BB) {
        for (MachineOperand &MO : I.operands()) {
          if (!MO.isReg())
            continue;
          MCRegister R = MO.getReg();
          if (R == MOS6502::SP || R == MOS6502::SPlo || R == MOS6502::SPhi)
            UsesSP = true;
          else if (MOS6502::ZPRegClass.contains(R))
            HighestZP = std::max(HighestZP, R);
          else if (MOS6502::ZP_PTRRegClass.contains(R))
            HighestZPPtr = std::max(HighestZPPtr, R);
        }
      }
    }
  }

  const auto ZPIndex = [](MCRegister ZP) -> int {
    if (!ZP.isValid())
      return -1;
    assert(MOS6502::ZPRegClass.contains(ZP));
    return ZP - MOS6502::ZP_0;
  };
  const auto ZPPtrIndex = [](MCRegister ZPPtr) -> int {
    if (!ZPPtr.isValid())
      return -1;
    assert(MOS6502::ZP_PTRRegClass.contains(ZPPtr));
    return ZPPtr - MOS6502::ZP_PTR_0;
  };

  if (HighestZP.isValid())
    LLVM_DEBUG(dbgs() << "Highest ZP register used: " << ZPIndex(HighestZP)
                      << "\n");
  else
    LLVM_DEBUG(dbgs() << "No ZP registers used.\n");

  if (HighestZPPtr.isValid())
    LLVM_DEBUG(dbgs() << "Highest ZP_PTR register used: "
                      << ZPPtrIndex(HighestZPPtr) << "\n");
  else
    LLVM_DEBUG(dbgs() << "No ZP_PTR registers used.\n");

  if (UsesSP)
    LLVM_DEBUG(dbgs() << "Uses SP register.\n");
  else
    LLVM_DEBUG(dbgs() << "Does not use SP register.\n");

  Type *WordTy = Type::getInt16Ty(M.getContext());
  Type *ByteTy = Type::getInt8Ty(M.getContext());
  MCRegister ZP = MOS6502::ZP_0;

  auto ZPAddrs = std::make_unique<GlobalValue *[]>(MRI.getNumRegs() + 1);
  auto ZPPtrAddrs = std::make_unique<GlobalValue *[]>(MRI.getNumRegs() + 1);

  for (MCRegister ZPPtr = MOS6502::ZP_PTR_0; ZPPtr <= HighestZPPtr;
       ZPPtr = ZPPtr + 1) {
    GlobalVariable *Addr = new GlobalVariable(
        M, WordTy, /*isConstant=*/false, GlobalValue::PrivateLinkage,
        UndefValue::get(WordTy), GetZPName(ZPPtr, MRI),
        /*InsertBefore=*/nullptr, GlobalValue::NotThreadLocal,
        /*AddressSpace=*/1);

    ZPPtrAddrs[ZPPtr] = Addr;
    LLVM_DEBUG(dbgs() << "Adding ptr:\t" << *Addr << "\n");

    GlobalAlias *Lo;
    if (ZP <= HighestZP) {
      Lo = GlobalAlias::create(
          ByteTy, /*AddressSpace=*/1, GlobalValue::PrivateLinkage,
          GetZPName(ZP, MRI),
          ConstantExpr::getBitCast(Addr, ByteTy->getPointerTo(/*AddrSpace=*/1)),
          &M);
      ZPAddrs[ZP] = Lo;
      LLVM_DEBUG(dbgs() << "Adding lo:\t" << *Lo);
      ZP = ZP + 1;
    }
    if (ZP <= HighestZP) {
      GlobalAlias *Hi =
          GlobalAlias::create(ByteTy, /*AddressSpace=*/1,
                              GlobalValue::PrivateLinkage, GetZPName(ZP, MRI),
                              ConstantExpr::getGetElementPtr(
                                  ByteTy, Lo, ConstantInt::get(WordTy, 1)),
                              &M);
      LLVM_DEBUG(dbgs() << "Adding hi:\t" << *Hi);
      ZPAddrs[ZP] = Hi;
      ZP = ZP + 1;
    }
  }

  for (; ZP <= HighestZP; ZP = ZP + 1) {
    GlobalVariable *Addr = new GlobalVariable(
        M, ByteTy, /*isConstant=*/false, GlobalValue::PrivateLinkage,
        UndefValue::get(ByteTy), GetZPName(ZP, MRI),
        /*InsertBefore=*/nullptr, GlobalValue::NotThreadLocal,
        /*AddressSpace=*/1);
    ZPAddrs[ZP] = Addr;
    LLVM_DEBUG(dbgs() << "Adding standalone zp:\t" << *Addr << "\n");
  }

  if (UsesSP) {
    auto *Addr = new GlobalVariable(
                                    M, WordTy, /*isConstant=*/false, GlobalValue::ExternalLinkage,
                                    UndefValue::get(WordTy), "_SP",
                                    /*InsertBefore=*/nullptr, GlobalValue::NotThreadLocal,
                                    /*AddressSpace=*/1);
    LLVM_DEBUG(dbgs() << "Adding SP:\t" << *Addr << "\n");
    auto *Lo = GlobalAlias::create(
                                   ByteTy, /*AddressSpace=*/1, GlobalValue::PrivateLinkage, "_SPlo",
                                   ConstantExpr::getBitCast(Addr, ByteTy->getPointerTo(/*AddrSpace=*/1)),
                                   &M);
    LLVM_DEBUG(dbgs() << "Adding lo:\t" << *Lo);
    auto *Hi = GlobalAlias::create(
                                   ByteTy, /*AddressSpace=*/1, GlobalValue::PrivateLinkage, "_SPhi",
                                   ConstantExpr::getGetElementPtr(ByteTy, Lo, ConstantInt::get(WordTy, 1)),
                                   &M);
    LLVM_DEBUG(dbgs() << "Adding hi:\t" << *Hi);

    ZPPtrAddrs[MOS6502::SP] = Addr;
    ZPAddrs[MOS6502::SPlo] = Lo;
    ZPAddrs[MOS6502::SPhi] = Hi;
  }

  LLVM_DEBUG(dbgs() << "Replacing ZP pseudoinstructions.\n");

  const auto ChangeZPOperand = [&](MachineOperand &MO) {
    MO.ChangeToGA(ZPAddrs[MO.getReg()], /*Offset=*/0);
  };
  const auto ChangeZPPtrOperand = [&](MachineOperand &MO) {
    MO.ChangeToGA(ZPPtrAddrs[MO.getReg()], /*Offset=*/0);
  };

  for (Function &F : M.functions()) {
    MachineFunction &MF = MMI.getOrCreateMachineFunction(F);
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
    for (MachineBasicBlock &BB : MF) {
      for (MachineInstr &I : BB) {
        if (!I.isPseudo() || I.isInlineAsm())
          continue;

        LLVM_DEBUG(dbgs() << "ZP Pseudo:\t" << I);
        switch (I.getOpcode()) {
        default:
          llvm_unreachable("Unhandled pseudoinstruction.");
        case MOS6502::LDzpr:
          I.setDesc(TII.get(MOS6502::LDzp));
          ChangeZPOperand(I.getOperand(1));
          break;
        case MOS6502::LDAyindirr:
          I.setDesc(TII.get(MOS6502::LDAyindir));
          ChangeZPPtrOperand(I.getOperand(0));
          break;
        case MOS6502::STzpr:
          I.setDesc(TII.get(MOS6502::STzp));
          ChangeZPOperand(I.getOperand(0));
          while (I.getNumOperands() > 2)
            I.RemoveOperand(I.getNumOperands() - 1);
          break;
        }
        LLVM_DEBUG(dbgs() << "Replaced with:\t" << I);
      }
    }
  }

  return false;
}

void MOS6502LowerZPReg::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MachineModuleInfoWrapperPass>();
  AU.addPreserved<MachineModuleInfoWrapperPass>();
}

char MOS6502LowerZPReg::ID = 0;

INITIALIZE_PASS(MOS6502LowerZPReg, DEBUG_TYPE,
                "Lower Zero Page 'registers' to memory locations", false, false)

ModulePass *llvm::createMOS6502LowerZPReg() { return new MOS6502LowerZPReg(); }

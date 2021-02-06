#include "MOS6502StaticStackAlloc.h"

#include "MOS6502.h"
#include "MOS6502FrameLowering.h"
#include "MOS6502Subtarget.h"

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/ErrorHandling.h"

#define DEBUG_TYPE "mos6502-static-stack-alloc"

using namespace llvm;

namespace {

class MOS6502StaticStackAlloc : public ModulePass {
public:
  static char ID;

  MOS6502StaticStackAlloc() : ModulePass(ID) {
    llvm::initializeMOS6502StaticStackAllocPass(
        *PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};

void MOS6502StaticStackAlloc::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MachineModuleInfoWrapperPass>();
  AU.addPreserved<MachineModuleInfoWrapperPass>();
}

bool MOS6502StaticStackAlloc::runOnModule(Module &M) {
  MachineModuleInfo &MMI = getAnalysis<MachineModuleInfoWrapperPass>().getMMI();

  bool Changed = false;
  for (Function &F : M) {
    MachineFunction *MF = MMI.getMachineFunction(F);
    if (!MF)
      continue;

    const MOS6502FrameLowering &TFL =
        *MF->getSubtarget<MOS6502Subtarget>().getFrameLowering();

    uint64_t Size = TFL.staticSize(MF->getFrameInfo());
    if (!Size)
      continue;

    LLVM_DEBUG(dbgs() << "Found static stack for " << F.getName() << "\n");
    LLVM_DEBUG(dbgs() << "Size " << Size << "\n");

    Type *Typ = ArrayType::get(Type::getInt8Ty(M.getContext()), Size);
    GlobalVariable *Stack =
        new GlobalVariable(M, Typ, false, GlobalValue::PrivateLinkage,
                           UndefValue::get(Typ), Twine(F.getName()) + "_sstk");
    LLVM_DEBUG(dbgs() << "Allocated: " << *Stack << "\n");
    Changed = true;

    for (MachineBasicBlock &MBB : *MF) {
      for (MachineInstr &MI : MBB) {
        for (MachineOperand &MO : MI.operands()) {
          if (!MO.isTargetIndex())
            continue;
          MO.ChangeToGA(Stack, MO.getOffset(), MO.getTargetFlags());
        }
      }
    }
  }
  return Changed;
}

} // namespace

char MOS6502StaticStackAlloc::ID = 0;

INITIALIZE_PASS(MOS6502StaticStackAlloc, DEBUG_TYPE,
                "Allocate non-recursive stack to static memory", false, false)

ModulePass *llvm::createMOS6502StaticStackAllocPass() {
  return new MOS6502StaticStackAlloc();
}

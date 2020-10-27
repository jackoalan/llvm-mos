#include "MOS6502CallLowering.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"

#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <memory>

using namespace llvm;

bool MOS6502CallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                      const Value *Val,
                                      ArrayRef<Register> VRegs) const {
  if (!Val) return true;

  MachineFunction &MF = MIRBuilder.getMF();
  const Function &F = MF.getFunction();
  const auto &DL = F.getParent()->getDataLayout();

  SmallVector<LLT, 4> SplitTys;
  computeValueLLTs(DL, *Val->getType(), SplitTys, nullptr);

  assert(SplitTys.size() == VRegs.size() &&
         "Number of VRegs must equal number of types.");

  MachineInstrBuilder Instr = MIRBuilder.buildInstrNoInsert(MOS6502::RETURN);

  for (size_t i = 0; i < VRegs.size(); ++i) {
    const LLT &Ty = SplitTys[i];
    const Register VReg = VRegs[i];
    assert(Ty.isByteSized() && "Must divide evenly into bytes.");
    unsigned NumBytes = Ty.getSizeInBytes();

    if (NumBytes == 1) {
      Instr.addUse(VReg);
      continue;
    }

    SmallVector<Register, 4> ExpandedVRegs;
    LLT s8 = LLT::scalar(8);
    for (unsigned j = 0; j < NumBytes; ++j) {
      const Register VReg = MIRBuilder.getMRI()->createGenericVirtualRegister(s8);
      Instr.addUse(VReg);
      ExpandedVRegs.push_back(VReg);
    }
    MIRBuilder.buildUnmerge(ExpandedVRegs, VReg);
  }

  MIRBuilder.insertInstr(Instr);
  return true;
}

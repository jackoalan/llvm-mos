#include "MOS6502CallLowering.h"
#include "MOS6502CallingConv.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"

#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include <memory>

using namespace llvm;

namespace {

struct MOS6502OutgoingValueHandler : CallLowering::OutgoingValueHandler {
  MachineInstrBuilder& MIB;

  MOS6502OutgoingValueHandler(MachineIRBuilder &MIRBuilder,
                              MachineInstrBuilder& MIB,
                              MachineRegisterInfo &MRI, CCAssignFn *AssignFn) :
    OutgoingValueHandler(MIRBuilder, MRI, AssignFn), MIB(MIB) {}

  Register getStackAddress(uint64_t Size, int64_t Offset,
                           MachinePointerInfo &MPO) override {
    auto &MFI = MIRBuilder.getMF().getFrameInfo();
    int FI = MFI.CreateFixedObject(Size, Offset, true);
    return MIRBuilder.buildFrameIndex(LLT::pointer(0, 16), FI).getReg(0);
  }

  void assignValueToReg(Register ValVReg, Register PhysReg,
                        CCValAssign &VA) override {
    MIB.addUse(PhysReg, RegState::Implicit);
    Register ExtendReg = extendRegister(ValVReg, VA);
    MIRBuilder.buildCopy(PhysReg, ExtendReg);
  }

  void assignValueToAddress(Register ValVReg, Register Addr, uint64_t Size,
                            MachinePointerInfo &MPO, CCValAssign &VA) override {
    MachineFunction &MF = MIRBuilder.getMF();
    auto *MMO =
      MF.getMachineMemOperand(MPO,
                              MachineMemOperand::MOStore,
                              Size, inferAlignFromPtrInfo(MF, MPO));
    MIRBuilder.buildStore(ValVReg, Addr, *MMO);
  }
};

}  // namespace

bool MOS6502CallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                      const Value *Val,
                                      ArrayRef<Register> VRegs) const {
  if (!Val) llvm_unreachable("Not yet implemented.");

  MachineFunction &MF = MIRBuilder.getMF();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetLowering &TLI = *getTLI();
  const DataLayout &DL = MF.getDataLayout();
  LLVMContext &Ctx = Val->getContext();
  const Function &F = MF.getFunction();

  SmallVector<EVT, 4> ValueVTs;
  ComputeValueVTs(TLI, DL, Val->getType(), ValueVTs);
  assert(ValueVTs.size() == VRegs.size() && "Need one type for each VReg.");

  auto MIB = MIRBuilder.buildInstrNoInsert(MOS6502::RTS);
  MOS6502OutgoingValueHandler Handler(MIRBuilder, MIB, MRI, RetCC_MOS6502);
  SmallVector<ArgInfo, 4> Args;
  for (size_t Idx = 0; Idx < VRegs.size(); ++Idx) {
    Args.emplace_back(VRegs[Idx], ValueVTs[Idx].getTypeForEVT(Ctx));
    setArgFlags(Args.back(), AttributeList::ReturnIndex, DL, F);
  }

  bool Success = handleAssignments(MIRBuilder, Args, Handler);
  MIRBuilder.insertInstr(MIB);
  return Success;
}

#include "MOS6502CallLowering.h"
#include "MOS6502CallingConv.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"

#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <memory>

using namespace llvm;

namespace {

struct MOS6502OutgoingValueHandler : CallLowering::OutgoingValueHandler {
  MOS6502OutgoingValueHandler(MachineIRBuilder &MIRBuilder,
                              MachineRegisterInfo &MRI, CCAssignFn *AssignFn) :
    OutgoingValueHandler(MIRBuilder, MRI, AssignFn) {}

  Register getStackAddress(uint64_t Size, int64_t Offset,
                           MachinePointerInfo &MPO) override {
    llvm_unreachable("Not yet implemented");
  }

  void assignValueToReg(Register ValVReg, Register PhysReg,
                        CCValAssign &VA) override {
    llvm_unreachable("Not yet implemented");
  }

  void assignValueToAddress(Register ValVReg, Register Addr, uint64_t Size,
                            MachinePointerInfo &MPO, CCValAssign &VA) override {
    llvm_unreachable("Not yet implemented");
  }
};

}  // namespace

bool MOS6502CallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                      const Value *Val,
                                      ArrayRef<Register> VRegs) const {
  if (!Val) return true;

  MachineFunction &MF = MIRBuilder.getMF();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  MOS6502OutgoingValueHandler Handler(MIRBuilder, MRI, CC_MOS6502);
  SmallVector<ArgInfo, 4> Args;
  return handleAssignments(MIRBuilder, Args, Handler);

  return true;
}

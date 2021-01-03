#include "MOS6502CallLowering.h"
#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502CallingConv.h"

#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/Target/TargetMachine.h"
#include <memory>

using namespace llvm;

#define DEBUG_TYPE "mos6502-call-lowering"

namespace {

struct MOS6502OutgoingValueHandler : CallLowering::OutgoingValueHandler {
  // The instruction causing control flow to leave the current function.
  MachineInstrBuilder &MIB;

  MOS6502OutgoingValueHandler(MachineIRBuilder &MIRBuilder,
                              MachineInstrBuilder &MIB,
                              MachineRegisterInfo &MRI, CCAssignFn *AssignFn)
      : OutgoingValueHandler(MIRBuilder, MRI, AssignFn), MIB(MIB) {}

  Register getStackAddress(uint64_t Size, int64_t Offset,
                           MachinePointerInfo &MPO) override {
    report_fatal_error("Not yet implemented.");
  }

  void assignValueToReg(Register ValVReg, Register PhysReg,
                        CCValAssign &VA) override {
    switch (VA.getLocVT().getSizeInBits()) {
    default:
      llvm_unreachable("Bad register size.");
    case 8:
    case 16:
      break;
    }

    // Ensure that the physical remains alive until control flow leaves.
    MIB.addUse(PhysReg, RegState::Implicit);
    MIRBuilder.buildCopy(PhysReg, ValVReg);
  }

  void assignValueToAddress(Register ValVReg, Register Addr, uint64_t Size,
                            MachinePointerInfo &MPO, CCValAssign &VA) override {
    report_fatal_error("Not yet implemented.");
  }
};

struct MOS6502IncomingValueHandler : CallLowering::IncomingValueHandler {
  std::function<void(Register Reg)> MakeLive;

  MOS6502IncomingValueHandler(MachineIRBuilder &MIRBuilder,
                              MachineRegisterInfo &MRI, CCAssignFn *AssignFn,
                              std::function<void(Register Reg)> MakeLive)
      : IncomingValueHandler(MIRBuilder, MRI, AssignFn), MakeLive(MakeLive) {}

  Register getStackAddress(uint64_t Size, int64_t Offset,
                           MachinePointerInfo &MPO) override {
    report_fatal_error("Not yet implemented.");
  }

  void assignValueToReg(Register ValVReg, Register PhysReg,
                        CCValAssign &VA) override {
    switch (VA.getLocVT().getSizeInBits()) {
    default:
      llvm_unreachable("Bad register size.");
    case 8:
    case 16:
      break;
    }

    // Ensure that the physical is alive on function entry.
    MakeLive(PhysReg);
    MIRBuilder.buildCopy(ValVReg, PhysReg);
  }

  void assignValueToAddress(Register ValVReg, Register Addr, uint64_t Size,
                            MachinePointerInfo &MPO, CCValAssign &VA) override {
    report_fatal_error("Not yet implemented.");
  }
};

} // namespace

bool MOS6502CallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                      const Value *Val,
                                      ArrayRef<Register> VRegs) const {
  auto Return = MIRBuilder.buildInstrNoInsert(MOS6502::RTS);

  if (Val) {
    MachineFunction &MF = MIRBuilder.getMF();
    MachineRegisterInfo &MRI = MF.getRegInfo();
    const TargetLowering &TLI = *getTLI();
    const DataLayout &DL = MF.getDataLayout();
    LLVMContext &Ctx = Val->getContext();
    const Function &F = MF.getFunction();

    // Initialize data structures for TableGen calling convention compatibility
    // layer.
    SmallVector<EVT> ValueVTs;
    ComputeValueVTs(TLI, DL, Val->getType(), ValueVTs);
    assert(ValueVTs.size() == VRegs.size() && "Need one type for each VReg.");
    SmallVector<ArgInfo> Args;
    for (size_t Idx = 0; Idx < VRegs.size(); ++Idx) {
      Args.emplace_back(VRegs[Idx], ValueVTs[Idx].getTypeForEVT(Ctx));
      setArgFlags(Args.back(), AttributeList::ReturnIndex, DL, F);
    }

    // Invoke TableGen compatibility layer. The return instruction will be
    // annotated with implicit uses of any live variables out of the function.
    MOS6502OutgoingValueHandler Handler(MIRBuilder, Return, MRI, RetCC_MOS6502);
    if (!handleAssignments(MIRBuilder, Args, Handler))
      return false;
  }

  // Insert the final return once the return values are in place.
  MIRBuilder.insertInstr(Return);
  return true;
}

bool MOS6502CallLowering::lowerFormalArguments(
    MachineIRBuilder &MIRBuilder, const Function &F,
    ArrayRef<ArrayRef<Register>> VRegs) const {
  MachineFunction &MF = MIRBuilder.getMF();
  const DataLayout &DL = MF.getDataLayout();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  // Initialize data structures for TableGen calling convention compatibility
  // layer.
  SmallVector<ArgInfo> SplitArgs;
  unsigned i = 0;
  for (auto &Arg : F.args()) {
    if (DL.getTypeStoreSize(Arg.getType()).isZero())
      continue;

    if (i >= VRegs.size())
      report_fatal_error("Incoming argument splitting not yet implemented.");
    ArgInfo OrigArg{VRegs[i], Arg.getType()};
    setArgFlags(OrigArg, i + AttributeList::FirstArgIndex, DL, F);

    splitToValueTypes(OrigArg, SplitArgs, DL);
    ++i;
  }

  // Invoke TableGen compatibility layer.
  const auto MakeLive = [&](Register PhysReg) {
    MIRBuilder.getMBB().addLiveIn(PhysReg);
  };
  MOS6502IncomingValueHandler Handler(MIRBuilder, MRI, CC_MOS6502, MakeLive);
  return handleAssignments(MIRBuilder, SplitArgs, Handler);
}

bool MOS6502CallLowering::lowerCall(MachineIRBuilder &MIRBuilder,
                                    CallLoweringInfo &Info) const {
  if (!Info.Callee.isGlobal() && !Info.Callee.isSymbol())
    report_fatal_error("Callee type not yet implemented.");
  if (Info.IsMustTailCall)
    report_fatal_error("Musttail calls not yet implemented.");
  if (Info.IsVarArg)
    report_fatal_error("Vararg calls not yet implemented.");

  MachineFunction &MF = MIRBuilder.getMF();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const DataLayout &DL = MF.getDataLayout();
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();

  auto Call = MIRBuilder.buildInstrNoInsert(MOS6502::JSR)
                  .add(Info.Callee)
                  .addRegMask(TRI.getCallPreservedMask(
                      MF, MF.getFunction().getCallingConv()));

  SmallVector<ArgInfo, 8> OutArgs;
  for (auto &OrigArg : Info.OrigArgs) {
    splitToValueTypes(OrigArg, OutArgs, DL);
  }

  SmallVector<ArgInfo, 8> InArgs;
  if (!Info.OrigRet.Ty->isVoidTy())
    splitToValueTypes(Info.OrigRet, InArgs, DL);

  // Invoke TableGen compatibility layer for outgoing arguments. The call
  // instruction will be annotated with implicit uses of any live variables out
  // of the function.
  MOS6502OutgoingValueHandler ArgsHandler(MIRBuilder, Call, MRI, CC_MOS6502);
  if (!handleAssignments(MIRBuilder, OutArgs, ArgsHandler))
    return false;

  // Insert the call once the outgoing arguments are in place.
  MIRBuilder.insertInstr(Call);

  // If the return value is void, there's nothing to do.
  if (Info.OrigRet.Ty->isVoidTy()) {
    assert(InArgs.empty());
    // Success.
    return true;
  }

  // Invoke TableGen compatibility layer for return value.
  const auto MakeLive = [&](Register PhysReg) {
    Call.addDef(PhysReg, RegState::Implicit);
  };
  MOS6502IncomingValueHandler RetHandler(MIRBuilder, MRI, RetCC_MOS6502,
                                         MakeLive);
  return handleAssignments(MIRBuilder, InArgs, RetHandler);
}

void MOS6502CallLowering::splitToValueTypes(const ArgInfo &OrigArg,
                                            SmallVectorImpl<ArgInfo> &SplitArgs,
                                            const DataLayout &DL) const {
  LLVMContext &Ctx = OrigArg.Ty->getContext();

  SmallVector<EVT, 4> SplitVTs;
  SmallVector<uint64_t, 4> Offsets;
  ComputeValueVTs(*getTLI(), DL, OrigArg.Ty, SplitVTs, &Offsets, 0);

  if (SplitVTs.size() == 0)
    return;

  if (SplitVTs.size() == 1) {
    // No splitting to do, but we want to replace the original type (e.g. [1 x
    // double] -> double).
    SplitArgs.emplace_back(OrigArg.Regs[0], SplitVTs[0].getTypeForEVT(Ctx),
                           OrigArg.Flags[0], OrigArg.IsFixed);
    return;
  }

  // Create one ArgInfo for each virtual register in the original ArgInfo.
  assert(OrigArg.Regs.size() == SplitVTs.size() && "Regs / types mismatch");

  for (unsigned i = 0, e = SplitVTs.size(); i < e; ++i) {
    Type *SplitTy = SplitVTs[i].getTypeForEVT(Ctx);
    SplitArgs.emplace_back(OrigArg.Regs[i], SplitTy, OrigArg.Flags[0],
                           OrigArg.IsFixed);
  }
}

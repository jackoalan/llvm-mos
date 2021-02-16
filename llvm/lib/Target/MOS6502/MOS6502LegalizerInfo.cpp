#include "MOS6502LegalizerInfo.h"

#include "MCTargetDesc/MOS6502MCTargetDesc.h"
#include "MOS6502MachineFunctionInfo.h"

#include "llvm/CodeGen/GlobalISel/LegalizerHelper.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

MOS6502LegalizerInfo::MOS6502LegalizerInfo() {
  using namespace TargetOpcode;
  using namespace LegalityPredicates;
  using namespace LegalizeMutations;

  LLT s1 = LLT::scalar(1);
  LLT s8 = LLT::scalar(8);
  LLT s16 = LLT::scalar(16);
  LLT s32 = LLT::scalar(32);
  LLT s64 = LLT::scalar(64);
  LLT p = LLT::pointer(0, 16);

  // Constants

  getActionDefinitionsBuilder(G_IMPLICIT_DEF)
      .legalFor({s1, s8, p})
      .clampScalar(0, s8, s8);

  getActionDefinitionsBuilder(G_CONSTANT)
      .legalFor({s1, s8})
      .clampScalar(0, s8, s8);

  getActionDefinitionsBuilder({G_FRAME_INDEX, G_GLOBAL_VALUE}).legalFor({p});

  // Integer Extension and Truncation

  // Narrowing ZEXT to 8 bits should remove it entirely.
  getActionDefinitionsBuilder(G_ZEXT).clampScalar(0, s8, s8).unsupported();

  // Type Conversions

  getActionDefinitionsBuilder(G_INTTOPTR).legalFor({{p, s16}});
  getActionDefinitionsBuilder(G_PTRTOINT).legalFor({{s16, p}});

  // Scalar Operations

  getActionDefinitionsBuilder(G_MERGE_VALUES)
      .legalForCartesianProduct({s16, p}, {s8});

  getActionDefinitionsBuilder(G_UNMERGE_VALUES)
      .legalForCartesianProduct({s8}, {s16, p});

  // Integer Operations

  getActionDefinitionsBuilder({G_ADD, G_OR, G_XOR})
      .legalFor({s8})
      .clampScalar(0, s8, s8);

  getActionDefinitionsBuilder(
      {G_SDIV, G_SREM, G_UDIV, G_UREM, G_CTLZ_ZERO_UNDEF})
      .libcall();

  getActionDefinitionsBuilder(G_SHL).customFor({s8, s16, s32, s64});

  getActionDefinitionsBuilder(G_ICMP).legalFor({{s1, s8}});

  getActionDefinitionsBuilder(G_PTR_ADD).legalFor({{p, s8}}).customFor(
      {{p, s16}});

  getActionDefinitionsBuilder(G_UADDO).customFor({s8});
  getActionDefinitionsBuilder(G_UADDE).legalFor({s8});

  // Floating Point Operations

  getActionDefinitionsBuilder({G_FADD,       G_FSUB,
                               G_FMUL,       G_FDIV,
                               G_FMA,        G_FPOW,
                               G_FREM,       G_FCOS,
                               G_FSIN,       G_FLOG10,
                               G_FLOG,       G_FLOG2,
                               G_FEXP,       G_FEXP2,
                               G_FCEIL,      G_FFLOOR,
                               G_FMINNUM,    G_FMAXNUM,
                               G_FSQRT,      G_FRINT,
                               G_FNEARBYINT, G_INTRINSIC_ROUNDEVEN,
                               G_FPEXT,      G_FPTRUNC,
                               G_FPTOSI,     G_FPTOUI,
                               G_SITOFP,     G_UITOFP})
      .libcall();

  // Memory Operations

  getActionDefinitionsBuilder({G_LOAD, G_STORE})
      .legalFor({{s8, p}})
      .customFor({{p, p}})
      .clampScalar(0, s8, s8);

  getActionDefinitionsBuilder({G_MEMCPY, G_MEMMOVE, G_MEMSET}).libcall();

  // Control Flow

  getActionDefinitionsBuilder(G_PHI).legalFor({s8}).clampScalar(0, s8, s8);

  getActionDefinitionsBuilder(G_BRCOND).legalFor({s1});

  // Variadic Arguments

  getActionDefinitionsBuilder({G_VASTART, G_VAARG}).custom();

  computeTables();
}

bool MOS6502LegalizerInfo::legalizeCustom(LegalizerHelper &Helper,
                                          MachineInstr &MI) const {
  using namespace TargetOpcode;
  MachineRegisterInfo &MRI = MI.getMF()->getRegInfo();

  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("Invalid opcode for custom legalization.");
  case G_PTR_ADD:
    return legalizePtrAdd(Helper, MRI, MI);
  case G_LOAD:
    return legalizeLoad(Helper, MRI, MI);
  case G_SHL:
    return legalizeShl(Helper, MRI, MI);
  case G_STORE:
    return legalizeStore(Helper, MRI, MI);
  case G_UADDO:
    return legalizeUAddO(Helper, MRI, MI);
  case G_VAARG:
    return legalizeVAArg(Helper, MRI, MI);
  case G_VASTART:
    return legalizeVAStart(Helper, MRI, MI);
  }
}

bool MOS6502LegalizerInfo::legalizeLoad(LegalizerHelper &Helper,
                                        MachineRegisterInfo &MRI,
                                        MachineInstr &MI) const {
  using namespace TargetOpcode;

  assert(MI.getOpcode() == G_LOAD);

  MachineIRBuilder &Builder = Helper.MIRBuilder;
  Register Tmp = MRI.createGenericVirtualRegister(LLT::scalar(16));
  Builder.setInsertPt(Builder.getMBB(), std::next(Builder.getInsertPt()));
  Builder.buildIntToPtr(MI.getOperand(0), Tmp).getReg(1);
  Helper.Observer.changingInstr(MI);
  MI.getOperand(0).setReg(Tmp);
  Helper.Observer.changedInstr(MI);
  return true;
}

bool MOS6502LegalizerInfo::legalizePtrAdd(LegalizerHelper &Helper,
                                          MachineRegisterInfo &MRI,
                                          MachineInstr &MI) const {
  using namespace TargetOpcode;

  assert(MI.getOpcode() == G_PTR_ADD);

  MachineIRBuilder &Builder = Helper.MIRBuilder;

  MachineOperand &Result = MI.getOperand(0);
  MachineOperand &Base = MI.getOperand(1);
  MachineOperand &Offset = MI.getOperand(2);

  MachineInstr *GlobalBase = getOpcodeDef(G_GLOBAL_VALUE, Base.getReg(), MRI);
  auto ConstOffset = getConstantVRegValWithLookThrough(Offset.getReg(), MRI);

  // Fold constant offsets into global value operand.
  if (GlobalBase && ConstOffset) {
    const MachineOperand &Op = GlobalBase->getOperand(1);
    Builder.buildInstr(G_GLOBAL_VALUE)
        .add(Result)
        .addGlobalAddress(Op.getGlobal(),
                          Op.getOffset() + ConstOffset->Value.getSExtValue());
    MI.eraseFromParent();
    return true;
  }

  const auto HasOnlyLoadStoreUses = [&]() {
    for (MachineInstr &MI : MRI.use_nodbg_instructions(Result.getReg()))
      switch (MI.getOpcode()) {
      default:
        return false;
      case G_LOAD:
      case G_STORE:
        break;
      }
    return true;
  };
  if (HasOnlyLoadStoreUses()) {
    // Adds of zero-extended values can instead use the legal 8-bit version of
    // G_PTR_ADD, with the goal of selecting indexed addressing modes.
    MachineInstr *ZExtOffset = getOpcodeDef(G_ZEXT, Offset.getReg(), MRI);
    if (ZExtOffset) {
      Helper.Observer.changingInstr(MI);
      Offset.setReg(ZExtOffset->getOperand(1).getReg());
      Helper.Observer.changedInstr(MI);
      return true;
    }

    // Similarly for values that fit in 8-bit unsigned constants.
    if (ConstOffset && ConstOffset->Value.isNonNegative() &&
        ConstOffset->Value.getActiveBits() <= 8) {
      auto Const =
          Builder.buildConstant(LLT::scalar(8), ConstOffset->Value.trunc(8));
      Helper.Observer.changingInstr(MI);
      Offset.setReg(Const.getReg(0));
      Helper.Observer.changedInstr(MI);
      return true;
    }
  }

  // Generalized pointer additions must be lowered to 16-bit integer arithmetic.
  LLT s16 = LLT::scalar(16);
  Register PtrVal = Builder.buildPtrToInt(s16, MI.getOperand(1)).getReg(0);
  Register Sum = Builder.buildAdd(s16, PtrVal, MI.getOperand(2)).getReg(0);
  Builder.buildIntToPtr(MI.getOperand(0), Sum);
  MI.eraseFromParent();
  return true;
}

bool MOS6502LegalizerInfo::legalizeShl(LegalizerHelper &Helper,
                                       MachineRegisterInfo &MRI,
                                       MachineInstr &MI) const {
  using namespace TargetOpcode;

  assert(MI.getOpcode() == G_SHL);

  MachineIRBuilder &Builder = Helper.MIRBuilder;

  MachineOperand &Dst = MI.getOperand(0);
  MachineOperand &Src = MI.getOperand(1);
  MachineOperand &Amt = MI.getOperand(2);

  auto ConstantAmt = getConstantVRegValWithLookThrough(Amt.getReg(), MRI);
  if (!ConstantAmt || ConstantAmt->Value != 1) {
    return false;
  }

  LLT Ty = MRI.getType(Dst.getReg());
  assert(Ty == MRI.getType(Src.getReg()));
  assert(Ty.isByteSized());

  auto Unmerge = Builder.buildUnmerge(LLT::scalar(8), Src);
  bool IsFirst = true;
  SmallVector<Register> Parts;
  Register Carry;
  for (MachineOperand &SrcPart : Unmerge->defs()) {
    Parts.push_back(MRI.createGenericVirtualRegister(LLT::scalar(8)));
    Register NewCarry = MRI.createGenericVirtualRegister(LLT::scalar(1));
    if (IsFirst) {
      IsFirst = false;
      Builder.buildInstr(MOS6502::G_LSHO)
          .addDef(Parts.back())
          .addDef(NewCarry)
          .addUse(SrcPart.getReg());
    } else {
      Builder.buildInstr(MOS6502::G_LSHE)
          .addDef(Parts.back())
          .addDef(NewCarry)
          .addUse(SrcPart.getReg())
          .addUse(Carry);
    }
    Carry = NewCarry;
  }
  Builder.buildMerge(Dst, Parts);
  MI.eraseFromParent();

  return true;
}

bool MOS6502LegalizerInfo::legalizeStore(LegalizerHelper &Helper,
                                         MachineRegisterInfo &MRI,
                                         MachineInstr &MI) const {
  using namespace TargetOpcode;

  assert(MI.getOpcode() == G_STORE);

  MachineIRBuilder &Builder = Helper.MIRBuilder;
  Register Tmp =
      Builder.buildPtrToInt(LLT::scalar(16), MI.getOperand(0)).getReg(0);
  Helper.Observer.changingInstr(MI);
  MI.getOperand(0).setReg(Tmp);
  Helper.Observer.changedInstr(MI);
  return true;
}

bool MOS6502LegalizerInfo::legalizeUAddO(LegalizerHelper &Helper,
                                         MachineRegisterInfo &MRI,
                                         MachineInstr &MI) const {
  using namespace TargetOpcode;

  assert(MI.getOpcode() == G_UADDO);

  MachineIRBuilder &Builder = Helper.MIRBuilder;
  auto CarryIn = Builder.buildConstant(LLT::scalar(1), 0).getReg(0);
  Builder.buildUAdde(MI.getOperand(0), MI.getOperand(1), MI.getOperand(2),
                     MI.getOperand(3), CarryIn);
  MI.eraseFromParent();
  return true;
}

bool MOS6502LegalizerInfo::legalizeVAArg(LegalizerHelper &Helper,
                                         MachineRegisterInfo &MRI,
                                         MachineInstr &MI) const {
  using namespace TargetOpcode;

  assert(MI.getOpcode() == G_VAARG);

  MachineIRBuilder &Builder = Helper.MIRBuilder;
  MachineFunction &MF = Builder.getMF();

  Register Dst = MI.getOperand(0).getReg();
  Register VaListPtr = MI.getOperand(1).getReg();

  LLT p = LLT::pointer(0, 16);

  MachineMemOperand *AddrLoadMMO = MF.getMachineMemOperand(
      MachinePointerInfo::getUnknownStack(MF),
      MachineMemOperand::MOLoad | MachineMemOperand::MOInvariant, 2, Align());
  Register Addr = Builder.buildLoad(p, VaListPtr, *AddrLoadMMO).getReg(0);

  unsigned Size = MRI.getType(Dst).getSizeInBytes();
  MachineMemOperand *ValueMMO = MF.getMachineMemOperand(
      MachinePointerInfo::getUnknownStack(MF),
      MachineMemOperand::MOLoad | MachineMemOperand::MOInvariant, Size,
      Align());
  Builder.buildLoad(Dst, Addr, *ValueMMO);

  Register SizeReg = Builder.buildConstant(LLT::scalar(16), Size).getReg(0);

  Register NextAddr = Builder.buildPtrAdd(p, Addr, SizeReg).getReg(0);
  MachineMemOperand *AddrStoreMMO =
      MF.getMachineMemOperand(MachinePointerInfo::getUnknownStack(MF),
                              MachineMemOperand::MOStore, 2, Align());
  Builder.buildStore(NextAddr, VaListPtr, *AddrStoreMMO);
  MI.eraseFromParent();
  return true;
}

bool MOS6502LegalizerInfo::legalizeVAStart(LegalizerHelper &Helper,
                                           MachineRegisterInfo &MRI,
                                           MachineInstr &MI) const {
  using namespace TargetOpcode;

  assert(MI.getOpcode() == G_VASTART);

  MachineIRBuilder &Builder = Helper.MIRBuilder;
  auto *FuncInfo = Builder.getMF().getInfo<MOS6502FunctionInfo>();
  Register Addr = Builder
                      .buildFrameIndex(LLT::pointer(0, 16),
                                       FuncInfo->getVarArgsStackIndex())
                      .getReg(0);
  Builder.buildStore(Addr, MI.getOperand(0), **MI.memoperands_begin());
  MI.eraseFromParent();
  return true;
}
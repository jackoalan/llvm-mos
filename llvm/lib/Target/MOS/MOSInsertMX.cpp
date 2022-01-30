//===-- MOSInsertMX.cpp - MOS MX Insertion --------------------------------===//
//
// Part of LLVM-MOS, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the MOS MX insertion pass.
//
// 65816 instructions consider operand 0 as 8 or 16 bits wide depending on the
// state of the M (A/Mem) or X (X/Y) flag. If the flag is unset, the operand is
// 16 bits, otherwise 8. Prior to emitting assembly, the necessary SEP/REP
// instructions are inserted to
//
//===----------------------------------------------------------------------===//

#include "MOSInsertMX.h"

#include "MCTargetDesc/MOSMCTargetDesc.h"
#include "MOS.h"
#include "MOSRegisterInfo.h"

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"

#define DEBUG_TYPE "mos-insert-mx"

using namespace llvm;

namespace {

class MOSInsertMX : public MachineFunctionPass {
public:
  static char ID;

  MOSInsertMX() : MachineFunctionPass(ID) {
    llvm::initializeMOSInsertMXPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
};

enum { UsesMX = 0x1 };

enum {
  MSet = 0x1,
  MUnset = 0x2,
  MMask = MSet | MUnset,
  XSet = 0x4,
  XUnset = 0x8,
  XMask = XSet | XUnset,
  Set = MSet | XSet,
  Unset = MUnset | XUnset,
};

unsigned getRequiredFlagBit(Register Reg) {
  if (Reg >= MOS::RC0 && Reg <= MOS::RC255)
    return MSet;
  if (Reg >= MOS::RS0 && Reg <= MOS::RS127)
    return MUnset;

  switch (Reg) {
  case MOS::A:
    return MSet;
  case MOS::EA:
    return MUnset;
  case MOS::X:
  case MOS::Y:
    return XSet;
  case MOS::EX:
  case MOS::EY:
    return XUnset;
  default:
    llvm_unreachable("invalid register");
  }
}

bool MOSInsertMX::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;
  DenseMap<MachineBasicBlock *, unsigned> MBBFlagBits;

  ReversePostOrderTraversal<MachineFunction *> RPOT(&MF);
  for (MachineBasicBlock *MBB : RPOT) {
    unsigned FlagBits = 0;
    for (MachineBasicBlock *Pred : MBB->predecessors()) {
      auto I = MBBFlagBits.find(Pred);
      assert(I != MBBFlagBits.end() && "pred not visited");
      FlagBits |= I->second;
    }

    // Unset bits if there are conflicting predecessors.
    if ((FlagBits & MMask) == MMask)
      FlagBits &= ~MMask;
    if ((FlagBits & XMask) == XMask)
      FlagBits &= ~XMask;

    for (MachineInstr &MI : *MBB) {
      if ((MI.getDesc().TSFlags & UsesMX) == 0)
        continue;

      unsigned RequiredBit = getRequiredFlagBit(MI.getOperand(0).getReg());
      if (FlagBits & RequiredBit)
        continue;

      // M or X flag needs to change before this instruction.
      unsigned Opcode =
          RequiredBit & Set ? MOS::SEP_Immediate : MOS::REP_Immediate;
      unsigned Imm = RequiredBit & MMask ? 0x20 : 0x10;
      FlagBits &= ~(RequiredBit & MMask ? MMask : XMask);
      FlagBits |= RequiredBit;

      // TODO: Fold into prior SEP/REP if it exists.
      MachineIRBuilder Builder(MI);
      Builder.buildInstr(Opcode).addImm(Imm);
      Changed = true;
    }

    // Keep track of bits for successors to use.
    auto P = MBBFlagBits.insert({MBB, FlagBits});
    assert(P.second && "mbb already visited");
  }

  return Changed;
}

} // namespace

char MOSInsertMX::ID = 0;

INITIALIZE_PASS(MOSInsertMX, DEBUG_TYPE, "MOS MX Insertion", false, false)

MachineFunctionPass *llvm::createMOSInsertMXPass() { return new MOSInsertMX; }

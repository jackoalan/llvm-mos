//===-- MOSMCInstLower.cpp - Convert MOS MachineInstr to an MCInst --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower MOS MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "MOSMCInstLower.h"
#include "MCTargetDesc/MOSMCExpr.h"
#include "MOSInstrInfo.h"
#include "MOSRegisterInfo.h"
#include "MOSSubtarget.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "mos-mcinstlower"

void MOSMCInstLower::lower(const MachineInstr *MI, MCInst &OutMI) {
  OutMI.setOpcode(MI->getOpcode());

  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp;
    if (lowerOperand(MO, MCOp))
      OutMI.addOperand(MCOp);
  }
}

bool MOSMCInstLower::lowerOperand(const MachineOperand &MO,
                                      MCOperand &MCOp) {
  const MOSRegisterInfo &TRI = *MO.getParent()
                                        ->getMF()
                                        ->getSubtarget<MOSSubtarget>()
                                        .getRegisterInfo();

  switch (MO.getType()) {
  default:
    LLVM_DEBUG(dbgs() << "Operand: " << MO << "\n");
    report_fatal_error("Operand type not implemented.");
  case MachineOperand::MO_RegisterMask:
    return false;
  case MachineOperand::MO_ExternalSymbol:
    MCOp = MCOperand::createExpr(MCSymbolRefExpr::create(
        Ctx.getOrCreateSymbol(MO.getSymbolName()), Ctx));
    break;
  case MachineOperand::MO_GlobalAddress: {
    const MCExpr *Expr =
        MCSymbolRefExpr::create(AP.getSymbol(MO.getGlobal()), Ctx);
    if (MO.getOffset() != 0)
      Expr = MCBinaryExpr::createAdd(
          Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);
    Expr = applyTargetFlags(MO.getTargetFlags(), Expr);
    MCOp = MCOperand::createExpr(Expr);
    break;
  }
  case MachineOperand::MO_Immediate:
    MCOp = MCOperand::createImm(MO.getImm());
    break;
  case MachineOperand::MO_MachineBasicBlock:
    MCOp = MCOperand::createExpr(
        MCSymbolRefExpr::create(MO.getMBB()->getSymbol(), Ctx));
    break;
  case MachineOperand::MO_Register:
    // Ignore all implicit register operands.
    if (MO.isImplicit())
      return false;
    Register Reg = MO.getReg();
    if (MOS::ZP_PTRRegClass.contains(Reg) ||
        MOS::ZPRegClass.contains(Reg))
      MCOp = MCOperand::createExpr(MCSymbolRefExpr::create(
          Ctx.getOrCreateSymbol(TRI.getZPSymbolName(Reg)), Ctx));
    else
      MCOp = MCOperand::createReg(MO.getReg());
    break;
  }
  return true;
}

const MCExpr *MOSMCInstLower::applyTargetFlags(unsigned Flags,
                                                   const MCExpr *Expr) {
  switch (Flags) {
  default:
    llvm_unreachable("Invalid target operand flags.");
  case MOS::MO_NO_FLAGS:
    return Expr;
  case MOS::MO_LO:
    return MOSMCExpr::createLo(Expr, Ctx);
  case MOS::MO_HI:
    return MOSMCExpr::createHi(Expr, Ctx);
  }
}

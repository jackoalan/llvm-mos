#include "MOS6502MCInstLower.h"
#include "MCTargetDesc/MOS6502MCExpr.h"
#include "MOS6502InstrInfo.h"
#include "MOS6502RegisterInfo.h"
#include "MOS6502Subtarget.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "mos6502-mcinstlower"

void MOS6502MCInstLower::lower(const MachineInstr *MI, MCInst &OutMI) {
  OutMI.setOpcode(MI->getOpcode());

  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp;
    if (lowerOperand(MO, MCOp))
      OutMI.addOperand(MCOp);
  }
}

bool MOS6502MCInstLower::lowerOperand(const MachineOperand &MO,
                                      MCOperand &MCOp) {
  const MOS6502RegisterInfo &TRI = *MO.getParent()
                                        ->getMF()
                                        ->getSubtarget<MOS6502Subtarget>()
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
    if (MOS6502::ZP_PTRRegClass.contains(Reg) ||
        MOS6502::ZPRegClass.contains(Reg))
      MCOp = MCOperand::createExpr(MCSymbolRefExpr::create(
          Ctx.getOrCreateSymbol(TRI.getZPSymbolName(Reg)), Ctx));
    else
      MCOp = MCOperand::createReg(MO.getReg());
    break;
  }
  return true;
}

const MCExpr *MOS6502MCInstLower::applyTargetFlags(unsigned Flags,
                                                   const MCExpr *Expr) {
  switch (Flags) {
  default:
    llvm_unreachable("Invalid target operand flags.");
  case MOS6502::MO_NO_FLAGS:
    return Expr;
  case MOS6502::MO_LO:
    return MOS6502MCExpr::createLo(Expr, Ctx);
  case MOS6502::MO_HI:
    return MOS6502MCExpr::createHi(Expr, Ctx);
  }
}

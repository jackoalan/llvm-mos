#include "MOS6502MCInstLower.h"
#include "MCTargetDesc/MOS6502MCExpr.h"
#include "MOS6502InstrInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

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
  switch (MO.getType()) {
  default:
    report_fatal_error("Operand type not implemented.");
  case MachineOperand::MO_GlobalAddress: {
    const MCExpr *Expr =
        MCSymbolRefExpr::create(AP.getSymbol(MO.getGlobal()), Ctx);
    Expr = applyTargetFlags(MO.getTargetFlags(), Expr);
    MCOp = MCOperand::createExpr(Expr);
    break;
  }
  case MachineOperand::MO_Immediate:
    MCOp = MCOperand::createImm(MO.getImm());
    break;
  case MachineOperand::MO_Register:
    // Ignore all implicit register operands.
    if (MO.isImplicit())
      return false;
    MCOp = MCOperand::createReg(MO.getReg());
    break;
  }
  return true;
}

const MCExpr *MOS6502MCInstLower::applyTargetFlags(unsigned Flags,
                                                   const MCExpr *Expr) {
  switch (Flags) {
  case MOS6502::MO_NO_FLAGS:
    return Expr;
  case MOS6502::MO_LO:
    return MOS6502MCExpr::createLo(Expr, Ctx);
  case MOS6502::MO_HI:
    return MOS6502MCExpr::createHi(Expr, Ctx);
  }
  llvm_unreachable("Invalid target operand flags.");
}

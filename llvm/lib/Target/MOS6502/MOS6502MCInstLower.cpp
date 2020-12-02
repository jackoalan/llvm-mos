#include "MOS6502MCInstLower.h"
#include "MCTargetDesc/MOS6502MCExpr.h"
#include "MOS6502InstrInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

static const MCExpr *applyTargetFlags(unsigned Flags, const MCExpr *Expr, MCContext &Ctx) {
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

static bool LowerMOS6502MachineOperandToMCOperand(const MachineOperand &MO,
                                                  MCOperand &MCOp,
                                                  const AsmPrinter &AP,
                                                  MCContext &Ctx) {
  switch (MO.getType()) {
  default:
    report_fatal_error("Operand type not implemented.");
  case MachineOperand::MO_GlobalAddress: {
    const MCExpr *Expr = MCSymbolRefExpr::create(AP.getSymbol(MO.getGlobal()), Ctx);
    Expr = applyTargetFlags(MO.getTargetFlags(), Expr, Ctx);
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

void llvm::LowerMOS6502MachineInstrToMCInst(const MachineInstr *MI,
                                            MCInst &OutMI, const AsmPrinter &AP,
                                            MCContext &Ctx) {
  OutMI.setOpcode(MI->getOpcode());

  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp;
    if (LowerMOS6502MachineOperandToMCOperand(MO, MCOp, AP, Ctx))
      OutMI.addOperand(MCOp);
  }
}

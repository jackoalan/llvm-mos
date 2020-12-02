#include "MOS6502MCInstLower.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

static bool LowerMOS6502MachineOperandToMCOperand(const MachineOperand &MO,
                                                  MCOperand &MCOp,
                                                  const AsmPrinter &AP) {
  switch (MO.getType()) {
  default:
    report_fatal_error("Unknown operand type.");
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
                                            MCInst &OutMI,
                                            const AsmPrinter &AP) {
  OutMI.setOpcode(MI->getOpcode());

  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp;
    if (LowerMOS6502MachineOperandToMCOperand(MO, MCOp, AP))
      OutMI.addOperand(MCOp);
  }
}

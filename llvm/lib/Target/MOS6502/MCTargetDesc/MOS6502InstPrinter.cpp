#include "MOS6502InstPrinter.h"
#include "MOS6502MCTargetDesc.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "MOS6502GenAsmWriter.inc"

void MOS6502InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                   StringRef Annot, const MCSubtargetInfo &STI,
                                   raw_ostream &OS) {
  switch (MI->getOpcode()) {
  default:
    printInstruction(MI, Address, OS);
    break;
  case MOS6502::CMPimm: {
    OS << "\t";
    unsigned Reg = MI->getOperand(0).getReg();
    switch (Reg) {
    default:
      llvm_unreachable("Invalid CMPimm register.");
    case MOS6502::A:
      OS << "CMP\t#";
      break;
    case MOS6502::X:
      OS << "CPX\t#";
      break;
    case MOS6502::Y:
      OS << "CPY\t#";
      break;
    }
    printOperand(MI, 1, OS);
    break;
  }
  case MOS6502::LDCimm: {
    OS << "\t";
    int64_t Imm = MI->getOperand(0).getImm();
    switch (Imm) {
    default:
      llvm_unreachable("Invalid LDCimm immediate.");
    case 0:
      OS << "CLC";
      break;
    case 1:
      OS << "SEC";
      break;
    }
    break;
  }
  }
}

void MOS6502InstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                      raw_ostream &OS) {
  const MCOperand &MO = MI->getOperand(OpNo);
  if (MO.isReg()) {
    printRegName(OS, MO.getReg());
  } else if (MO.isImm()) {
    OS << MO.getImm();
  } else if (MO.isExpr()) {
    MO.getExpr()->print(OS, &MAI);
  } else {
    report_fatal_error("Unknown operand kind.");
  }
}

void MOS6502InstPrinter::printRegName(raw_ostream &O, unsigned RegNo) const {
  O << getRegisterName(RegNo);
}

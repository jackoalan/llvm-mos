#include "MOS6502MCExpr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "mos6502mcexpr"

const MOS6502MCExpr *MOS6502MCExpr::createLo(const MCExpr *Expr, MCContext &Ctx) {
  return new (Ctx) MOS6502MCExpr(Expr, VK_MOS6502_LO);
}

const MOS6502MCExpr *MOS6502MCExpr::createHi(const MCExpr *Expr, MCContext &Ctx) {
  return new (Ctx) MOS6502MCExpr(Expr, VK_MOS6502_HI);
}

void MOS6502MCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  switch (Kind) {
  case VK_MOS6502_Invalid:
    llvm_unreachable("Invalid MOS6502MCExpr kind.");
  case VK_MOS6502_LO:
    OS << "<";
    break;
  case VK_MOS6502_HI:
    OS << ">";
    break;
  }
  Expr->print(OS, MAI, /*InParens=*/true);
}

bool MOS6502MCExpr::evaluateAsRelocatableImpl(MCValue &Res,
                                            const MCAsmLayout *Layout,
                                            const MCFixup *Fixup) const {
  report_fatal_error("Not yet implemented.");
}

void MOS6502MCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*Expr);
}

MOS6502MCExpr::VariantKind MOS6502MCExpr::getVariantKindForName(StringRef name) {
  report_fatal_error("Not yet implemented.");
}

StringRef MOS6502MCExpr::getVariantKindName(VariantKind Kind) {
  report_fatal_error("Not yet implemented.");
}

void MOS6502MCExpr::fixELFSymbolsInTLSFixups(MCAssembler &Asm) const {
  report_fatal_error("Not yet implemented.");
}

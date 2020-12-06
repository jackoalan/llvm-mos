#ifndef LLVM_LIB_TARGET_MOS6502_MCTARGETDESC_MOS6502MCEXPR_H
#define LLVM_LIB_TARGET_MOS6502_MCTARGETDESC_MOS6502MCEXPR_H

#include "llvm/MC/MCExpr.h"

namespace llvm {

class StringRef;

class MOS6502MCExpr : public MCTargetExpr {
public:
  enum VariantKind {
    VK_MOS6502_Invalid,
    VK_MOS6502_LO,
    VK_MOS6502_HI,
  };

private:
  const MCExpr *Expr;
  const VariantKind Kind;

  explicit MOS6502MCExpr(const MCExpr *Expr, VariantKind Kind)
      : Expr(Expr), Kind(Kind) {}

public:
  static const MOS6502MCExpr *createLo(const MCExpr *Expr, MCContext &Ctx);
  static const MOS6502MCExpr *createHi(const MCExpr *Expr, MCContext &Ctx);

  void printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const override;
  bool evaluateAsRelocatableImpl(MCValue &Res, const MCAsmLayout *Layout,
                                 const MCFixup *Fixup) const override;
  void visitUsedExpr(MCStreamer &Streamer) const override;
  MCFragment *findAssociatedFragment() const override {
    return Expr->findAssociatedFragment();
  }

  void fixELFSymbolsInTLSFixups(MCAssembler &Asm) const override;

  static bool classof(const MCExpr *E) {
    return E->getKind() == MCExpr::Target;
  }

  static bool classof(const MOS6502MCExpr *) { return true; }

  static VariantKind getVariantKindForName(StringRef name);
  static StringRef getVariantKindName(VariantKind Kind);
};

} // end namespace llvm.

#endif

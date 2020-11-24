#include "MOS6502LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"

using namespace llvm;

MOS6502LegalizerInfo::MOS6502LegalizerInfo() {
  using namespace TargetOpcode;
  using namespace LegalityPredicates;
  using namespace LegalizeMutations;

  LLT s1 = LLT::scalar(1);
  LLT s8 = LLT::scalar(8);
  LLT s16 = LLT::scalar(16);
  LLT p = LLT::pointer(0, 16);
  getActionDefinitionsBuilder(G_BRCOND).legalFor({s1});

  getActionDefinitionsBuilder(G_CONSTANT)
      .legalFor({s8, p})
      .clampScalar(0, s8, s8);

  getActionDefinitionsBuilder(G_ICMP)
    .legalForCartesianProduct({s1}, {s8}, {s8})
    .bitcastIf(typeIs(1, p), changeTo(1, s16))
    .bitcastIf(typeIs(2, p), changeTo(2, s16))
    .clampScalar(1, s8, s8)
    .clampScalar(2, s8, s8);

  computeTables();
}

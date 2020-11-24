#include "MOS6502LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/TargetOpcodes.h"

using namespace llvm;

MOS6502LegalizerInfo::MOS6502LegalizerInfo() {
  using namespace TargetOpcode;
  using namespace LegalityPredicates;
  using namespace LegalizeMutations;

  LLT s1 = LLT::scalar(1);
  LLT s8 = LLT::scalar(8);
  LLT s16 = LLT::scalar(16);
  LLT p = LLT::pointer(0, 16);

  getActionDefinitionsBuilder({G_ADD, G_OR, G_XOR})
      .legalFor({s8})
      .clampScalar(0, s8, s8);

  getActionDefinitionsBuilder(G_BRCOND).legalFor({s1});

  getActionDefinitionsBuilder(G_CONSTANT)
      .legalFor({s8, p})
      .clampScalar(0, s8, s8);

  getActionDefinitionsBuilder({G_GLOBAL_VALUE, G_PHI}).alwaysLegal();

  getActionDefinitionsBuilder(G_ICMP)
      .legalFor({{s1, s8}})
      .narrowScalarIf(typeIs(1, p), changeTo(1, s8))
      .clampScalar(1, s8, s8);

  getActionDefinitionsBuilder(G_LOAD).legalFor({{s8, p}}).clampScalar(0, s8,
                                                                      s8);

  getActionDefinitionsBuilder({G_MERGE_VALUES, G_SEXT}).legalFor({{s16, s8}});

  getActionDefinitionsBuilder(G_PTR_ADD).legalForCartesianProduct({p}, {s8, s16});

  computeTables();
}

#include "MOS6502LegalizerInfo.h"

using namespace llvm;

MOS6502LegalizerInfo::MOS6502LegalizerInfo() {
  using namespace TargetOpcode;

  LLT s8 = LLT::scalar(8);
  LLT p = LLT::pointer(0, 16);
  getActionDefinitionsBuilder(G_CONSTANT)
    .legalFor({s8, p})
    .clampScalar(0, s8, s8);

  computeTables();
}

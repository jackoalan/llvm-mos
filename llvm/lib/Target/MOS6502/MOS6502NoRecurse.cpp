#include "MOS6502NoRecurse.h"

#include "MOS6502.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"

#define DEBUG_TYPE "mos6502-norecurse"

using namespace llvm;

namespace {

struct MOS6502NoRecurse : public CallGraphSCCPass {
  static char ID; // Pass identification, replacement for typeid

  MOS6502NoRecurse() : CallGraphSCCPass(ID) {
    initializeMOS6502NoRecursePass(*PassRegistry::getPassRegistry());
  }

  bool runOnSCC(CallGraphSCC &SCC) override;

  bool doInitialization(CallGraph &CG) override {
    LLVM_DEBUG(dbgs() << "**** MOS6502 NoRecurse Pass ****\n");
    // For the conservative recursion analysis, any external call may call any
    // externally-callable function.
    assert(CG.getCallsExternalNode()->empty());
    CG.getCallsExternalNode()->addCalledFunction(nullptr, CG.getExternalCallingNode());
    // Report unchanged, since the call graph will be returned to its original
    // condition on finalization.
    return false;
  }

  bool doFinalization(CallGraph &CG) override {
    // Remove the artificial edge added in initialization.
    CG.getCallsExternalNode()->removeAllCalledFunctions();
    return false;
  }
};

bool MOS6502NoRecurse::runOnSCC(CallGraphSCC &SCC) {
  LLVM_DEBUG(dbgs() << "Begin SCC:\n");
  for (const CallGraphNode *N : SCC) {
    LLVM_DEBUG(N->print(dbgs()));
  }
  LLVM_DEBUG(dbgs() << "End SCC:\n");

  // All nodes in SCCs with more than one node may be recursive. It's not
  // certain since CFG analysis is conservative, but there's no more
  // information to be gleaned from looking at the call graph, and other
  // sources of information are better used making the CFG analysis less
  // conservative.
  if (!SCC.isSingular())
    return false;

  const CallGraphNode &N = **SCC.begin();

  if (!N.getFunction() || N.getFunction()->isDeclaration() ||
      N.getFunction()->doesNotRecurse())
    return false;

  // Since the CFG analysis is conservative, any possible indirect recursion
  // involving N would have placed in an SCC with more than one node. Thus, N
  // is recursive iff it directly calls itself.
  bool CallsSelf = false;
  for (const CallGraphNode::CallRecord &CR : N) {
    if (CR.second == &N) {
      CallsSelf = true;
      break;
    }
  }

  if (CallsSelf)
    return false;

  LLVM_DEBUG(dbgs() << "Found new non-recursive function.\n");
  LLVM_DEBUG(N.print(dbgs()));

  // At this point, the function in N is known non-recursive.
  N.getFunction()->setDoesNotRecurse();
  return true;
}

} // namespace

char MOS6502NoRecurse::ID = 0;

INITIALIZE_PASS(MOS6502NoRecurse, DEBUG_TYPE,
                "Introduce 8-bit IVs for GEP indices", false, false)

CallGraphSCCPass *llvm::createMOS6502NoRecursePass() {
  return new MOS6502NoRecurse();
}

#include "MOS6502MCInstLower.h"
#include "TargetInfo/MOS6502TargetInfo.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {

class MOS6502AsmPrinter : public AsmPrinter {
  MOS6502MCInstLower InstLowering;
  SmallPtrSet<MCSymbol*, 32> ExternalSymbols;

public:
  explicit MOS6502AsmPrinter(TargetMachine &TM,
                             std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)), InstLowering(OutContext, *this) {}

  void emitInstruction(const MachineInstr *MI) override;

  void emitEndOfAsmFile(Module &) override;
};

void MOS6502AsmPrinter::emitInstruction(const MachineInstr *MI) {
  for (const MachineOperand &MO : MI->operands())
    if (MO.isSymbol())
      ExternalSymbols.insert(OutContext.getOrCreateSymbol(MO.getSymbolName()));

  MCInst Inst;
  InstLowering.lower(MI, Inst);
  EmitToStreamer(*OutStreamer, Inst);
}

void MOS6502AsmPrinter::emitEndOfAsmFile(Module &M) {
  // Unlike GAS, ca65 requires declaring all used symbols, even if external.
  for (const auto &S : ExternalSymbols)
    OutStreamer->emitSymbolAttribute(S, MCSA_Global);
  ExternalSymbols.clear();
}

} // namespace

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMOS6502AsmPrinter() {
  RegisterAsmPrinter<MOS6502AsmPrinter> X(getTheMOS6502Target());
}

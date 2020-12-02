#include "MOS6502MCInstLower.h"
#include "TargetInfo/MOS6502TargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {

class MOS6502AsmPrinter : public AsmPrinter {
  MOS6502MCInstLower InstLowering;

public:
  explicit MOS6502AsmPrinter(TargetMachine &TM,
                             std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)), InstLowering(OutContext, *this) {}

  void emitInstruction(const MachineInstr *MI) override;
};

void MOS6502AsmPrinter::emitInstruction(const MachineInstr *MI) {
  MCInst Inst;
  InstLowering.lower(MI, Inst);
  EmitToStreamer(*OutStreamer, Inst);
}

} // namespace

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMOS6502AsmPrinter() {
  RegisterAsmPrinter<MOS6502AsmPrinter> X(getTheMOS6502Target());
}

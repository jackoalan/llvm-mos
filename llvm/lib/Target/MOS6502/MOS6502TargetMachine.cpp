#include "MOS6502TargetMachine.h"

#include "MOS6502.h"
#include "MOS6502IndexIVPass.h"
#include "MOS6502MachineScheduler.h"
#include "MOS6502NoRecurse.h"
#include "MOS6502PreLegalizerCombiner.h"
#include "MOS6502PreRegAlloc.h"
#include "MOS6502TargetObjectFile.h"
#include "MOS6502TargetTransformInfo.h"
#include "TargetInfo/MOS6502TargetInfo.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelect.h"
#include "llvm/CodeGen/GlobalISel/Legalizer.h"
#include "llvm/CodeGen/GlobalISel/Localizer.h"
#include "llvm/CodeGen/GlobalISel/RegBankSelect.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMOS6502Target() {
  RegisterTargetMachine<MOS6502TargetMachine> X(getTheMOS6502Target());
  PassRegistry &PR = *PassRegistry::getPassRegistry();
  initializeGlobalISel(PR);
  initializeMOS6502IndexIVPass(PR);
  initializeMOS6502NoRecursePass(PR);
  initializeMOS6502PreRegAllocPass(PR);
  initializeMOS6502PreLegalizerCombinerPass(PR);
}

static const char Layout[] =
    "e-p:16:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-a:8:8-Fi8-n8";

static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  if (!RM.hasValue())
    return Reloc::Static;
  return *RM;
}

MOS6502TargetMachine::MOS6502TargetMachine(const Target &T, const Triple &TT,
                                           StringRef CPU, StringRef FS,
                                           const TargetOptions &Options,
                                           Optional<Reloc::Model> RM,
                                           Optional<CodeModel::Model> CM,
                                           CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, Layout, TT, CPU, FS, Options,
                        getEffectiveRelocModel(RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<MOS6502TargetObjectFile>()) {
  initAsmInfo();
  setGlobalISel(true);

  Subtarget = std::make_unique<MOS6502Subtarget>(TT, CPU, FS, *this);

  // Prevents fallback to SelectionDAG by allowing direct aborts.
  setGlobalISelAbort(GlobalISelAbortMode::Enable);
}

const MOS6502Subtarget *
MOS6502TargetMachine::getSubtargetImpl(const Function &F) const {
  return Subtarget.get();
}

TargetTransformInfo
MOS6502TargetMachine::getTargetTransformInfo(const Function &F) {
  return TargetTransformInfo(MOS6502TTIImpl(this, F));
}

void MOS6502TargetMachine::adjustPassManager(PassManagerBuilder &Builder) {
  Builder.addExtension(
      PassManagerBuilder::EP_LateLoopOptimizations,
      [](const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
        // Lower to 8-bit index induction variables wherever possible to help
        // generate indexed addressing modes.
        PM.add(createMOS6502IndexIVPass());
        // New induction variables may have been added.
        PM.add(createIndVarSimplifyPass());
      });
}

namespace {

class MOS6502PassConfig : public TargetPassConfig {
public:
  MOS6502PassConfig(MOS6502TargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  MOS6502TargetMachine &getMOS6502TargetMachine() const {
    return getTM<MOS6502TargetMachine>();
  }

  void addIRPasses() override;
  bool addIRTranslator() override;
  void addPreLegalizeMachineIR() override;
  bool addLegalizeMachineIR() override;
  bool addRegBankSelect() override;
  void addPreGlobalInstructionSelect() override;
  bool addGlobalInstructionSelect() override;
  void addMachineSSAOptimization() override;
  void addPreRegAlloc() override;
  void addPreEmitPass() override;

  ScheduleDAGInstrs *
  createMachineScheduler(MachineSchedContext *C) const override;

  std::unique_ptr<CSEConfigBase> getCSEConfig() const override;
};

} // namespace

TargetPassConfig *MOS6502TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new MOS6502PassConfig(*this, PM);
}

void MOS6502PassConfig::addIRPasses() {
  addPass(createMOS6502NoRecursePass());
  TargetPassConfig::addIRPasses();
}

bool MOS6502PassConfig::addIRTranslator() {
  addPass(new IRTranslator(getOptLevel()));
  return false;
}

void MOS6502PassConfig::addPreLegalizeMachineIR() {
  addPass(createMOS6502PreLegalizerCombiner());
}

bool MOS6502PassConfig::addLegalizeMachineIR() {
  addPass(new Legalizer());
  return false;
}

bool MOS6502PassConfig::addRegBankSelect() {
  addPass(new RegBankSelect());
  return false;
}

bool MOS6502PassConfig::addGlobalInstructionSelect() {
  addPass(new InstructionSelect());
  return false;
}

void MOS6502PassConfig::addMachineSSAOptimization() {
  // Ensures that phsyreg defs are appropriately tagged with "dead", allowing
  // later SSA optimizations to ignore them. It's a little odd that these passes
  // use dead annotations; I think they're generated by SelectionDAG emission,
  // but there doesn't seem to be anything in GlobalISel that produces them in a
  // uniform fashion.
  addPass(&LiveVariablesID);
  TargetPassConfig::addMachineSSAOptimization();
}

void MOS6502PassConfig::addPreRegAlloc() {
  addPass(createMOS6502PreRegAlloc());
}

void MOS6502PassConfig::addPreEmitPass() { addPass(&BranchRelaxationPassID); }

void MOS6502PassConfig::addPreGlobalInstructionSelect() {
  addPass(new Localizer);
}

ScheduleDAGInstrs *
MOS6502PassConfig::createMachineScheduler(MachineSchedContext *C) const {
  return new ScheduleDAGMILive(C, std::make_unique<MOS6502SchedStrategy>(C));
}

std::unique_ptr<CSEConfigBase> MOS6502PassConfig::getCSEConfig() const {
  return getStandardCSEConfigForOpt(TM->getOptLevel());
}

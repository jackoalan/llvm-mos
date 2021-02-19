//===-- MOSTargetMachine.cpp - Define TargetMachine for MOS ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the MOS specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//
#include "MOSTargetMachine.h"

#include "MOS.h"
#include "MOSCombiner.h"
#include "MOSIndexIVPass.h"
#include "MOSMachineScheduler.h"
#include "MOSNoRecurse.h"
#include "MOSPreRegAlloc.h"
#include "MOSStaticStackAlloc.h"
#include "MOSTargetObjectFile.h"
#include "MOSTargetTransformInfo.h"
#include "TargetInfo/MOSTargetInfo.h"
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
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMOSTarget() {
  RegisterTargetMachine<MOSTargetMachine> X(getTheMOSTarget());
  PassRegistry &PR = *PassRegistry::getPassRegistry();
  initializeGlobalISel(PR);
  initializeMOSCombinerPass(PR);
  initializeMOSIndexIVPass(PR);
  initializeMOSNoRecursePass(PR);
  initializeMOSPreRegAllocPass(PR);
  initializeMOSStaticStackAllocPass(PR);
}

static const char Layout[] =
    "e-p:16:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-a:8:8-Fi8-n8";

static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  if (!RM.hasValue())
    return Reloc::Static;
  return *RM;
}

MOSTargetMachine::MOSTargetMachine(const Target &T, const Triple &TT,
                                           StringRef CPU, StringRef FS,
                                           const TargetOptions &Options,
                                           Optional<Reloc::Model> RM,
                                           Optional<CodeModel::Model> CM,
                                           CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, Layout, TT, CPU, FS, Options,
                        getEffectiveRelocModel(RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<MOSTargetObjectFile>()) {
  initAsmInfo();
  setGlobalISel(true);

  Subtarget = std::make_unique<MOSSubtarget>(TT, CPU, FS, *this);

  // Prevents fallback to SelectionDAG by allowing direct aborts.
  setGlobalISelAbort(GlobalISelAbortMode::Enable);
}

const MOSSubtarget *
MOSTargetMachine::getSubtargetImpl(const Function &F) const {
  return Subtarget.get();
}

TargetTransformInfo
MOSTargetMachine::getTargetTransformInfo(const Function &F) {
  return TargetTransformInfo(MOSTTIImpl(this, F));
}

void MOSTargetMachine::adjustPassManager(PassManagerBuilder &Builder) {
  Builder.addExtension(
      PassManagerBuilder::EP_LateLoopOptimizations,
      [](const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
        // Lower to 8-bit index induction variables wherever possible to help
        // generate indexed addressing modes.
        PM.add(createMOSIndexIVPass());
        // New induction variables may have been added.
        PM.add(createIndVarSimplifyPass());
      });
}

namespace {

class MOSPassConfig : public TargetPassConfig {
public:
  MOSPassConfig(MOSTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  MOSTargetMachine &getMOSTargetMachine() const {
    return getTM<MOSTargetMachine>();
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
  void addPreSched2() override;
  void addPreEmitPass() override;

  ScheduleDAGInstrs *
  createMachineScheduler(MachineSchedContext *C) const override;

  std::unique_ptr<CSEConfigBase> getCSEConfig() const override;
};

} // namespace

TargetPassConfig *MOSTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new MOSPassConfig(*this, PM);
}

void MOSPassConfig::addIRPasses() {
  addPass(createMOSNoRecursePass());
  TargetPassConfig::addIRPasses();
}

bool MOSPassConfig::addIRTranslator() {
  addPass(new IRTranslator(getOptLevel()));
  return false;
}

void MOSPassConfig::addPreLegalizeMachineIR() {
  addPass(createMOSCombiner());
}

bool MOSPassConfig::addLegalizeMachineIR() {
  addPass(new Legalizer());
  return false;
}

bool MOSPassConfig::addRegBankSelect() {
  addPass(new RegBankSelect());
  return false;
}

void MOSPassConfig::addPreGlobalInstructionSelect() {
  addPass(createMOSCombiner());
  addPass(new Localizer());
}

bool MOSPassConfig::addGlobalInstructionSelect() {
  addPass(new InstructionSelect());
  return false;
}

void MOSPassConfig::addMachineSSAOptimization() {
  // Ensures that phsyreg defs are appropriately tagged with "dead", allowing
  // later SSA optimizations to ignore them. It's a little odd that these passes
  // use dead annotations; I think they're generated by SelectionDAG emission,
  // but there doesn't seem to be anything in GlobalISel that produces them in a
  // uniform fashion.
  addPass(&LiveVariablesID);
  TargetPassConfig::addMachineSSAOptimization();
}

void MOSPassConfig::addPreRegAlloc() {
  addPass(createMOSPreRegAlloc());
}

void MOSPassConfig::addPreSched2() {
  addPass(createMOSStaticStackAllocPass());
}

void MOSPassConfig::addPreEmitPass() { addPass(&BranchRelaxationPassID); }

ScheduleDAGInstrs *
MOSPassConfig::createMachineScheduler(MachineSchedContext *C) const {
  return new ScheduleDAGMILive(C, std::make_unique<MOSSchedStrategy>(C));
}

std::unique_ptr<CSEConfigBase> MOSPassConfig::getCSEConfig() const {
  return getStandardCSEConfigForOpt(TM->getOptLevel());
}

//===-- MOSSubtarget.cpp - MOS Subtarget Information ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the MOS specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "MOSSubtarget.h"

#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/TargetRegistry.h"

#include "MCTargetDesc/MOSMCTargetDesc.h"
#include "MOS.h"
#include "MOSTargetMachine.h"

#define DEBUG_TYPE "mos-subtarget"

#include "MOSCallLowering.h"
#include "MOSISelLowering.h"
#include "MOSInstructionSelector.h"
#include "MOSLegalizerInfo.h"
#include "MOSRegisterBankInfo.h"
#include "llvm/CodeGen/GlobalISel/InlineAsmLowering.h"
#include "llvm/CodeGen/MachineScheduler.h"

using namespace llvm;

#define DEBUG_TYPE "mos-subtarget"

#define GET_SUBTARGETINFO_CTOR
#include "MOSGenSubtargetInfo.inc"

MOSSubtarget::MOSSubtarget(const Triple &TT, StringRef CPU,
                                   StringRef FS, const TargetMachine &TM)
    : MOSGenSubtargetInfo(TT, CPU, /*TuneCPU=*/CPU, FS),
      TLInfo(TM, initializeSubtargetDependencies(TT, CPU, CPU, FS)) {
  CallLoweringInfo.reset(new MOSCallLowering(getTargetLowering()));
  Legalizer.reset(new MOSLegalizerInfo);

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "MOSGenSubtargetInfo.inc"

namespace llvm {

MOSSubtarget::MOSSubtarget(const Triple &TT, const std::string &CPU,
                           const std::string &FS, const MOSTargetMachine &TM)
    : MOSGenSubtargetInfo(TT, CPU, /* TuneCPU */ CPU, FS), InstrInfo(),
      FrameLowering(), TLInfo(TM, initializeSubtargetDependencies(CPU, FS, TM)),
      TSInfo(),

      // Subtarget features
      m_hasTinyEncoding(false),

      m_Has6502Insns(false), m_Has6502BCDInsns(false), m_Has6502XInsns(false),
      m_Has65C02Insns(false), m_HasR65C02Insns(false), m_HasW65C02Insns(false),
      m_HasW65816Insns(false), m_Has65EL02Insns(false), m_Has65CE02Insns(false),
      m_HasSWEET16Insns(false),
      
      m_LongRegisterNames(false),

      ELFArch(0), m_FeatureSetDummy(false) {
  // Parse features string.
  ParseSubtargetFeatures(CPU, /* TuneCPU */ CPU, FS);
  CallLoweringInfo.reset(new MOSCallLowering(getTargetLowering()));
  Legalizer.reset(new MOSLegalizerInfo);
  auto *RBI = new MOSRegisterBankInfo;
  RegBankInfo.reset(RBI);
  InstSelector.reset(createMOSInstructionSelector(
      *static_cast<const MOSTargetMachine *>(&TM), *this, *RBI));
  InlineAsmLoweringInfo.reset(new InlineAsmLowering(getTargetLowering()));
}

MOSSubtarget &MOSSubtarget::initializeSubtargetDependencies(
    const Triple &TT, StringRef CPU, StringRef TuneCPU, StringRef FS) {
  ParseSubtargetFeatures(CPU, /* TuneCPU */ CPU, FS);
  std::string CPUName = std::string(CPU);
  std::string TuneCPUName = std::string(TuneCPU);
  if (CPUName.empty())
    CPUName = "generic";
  if (TuneCPUName.empty())
    TuneCPUName = CPUName;
  InitMCProcessorInfo(CPUName, TuneCPUName, FS);

  return *this;
}

void MOSSubtarget::overrideSchedPolicy(MachineSchedPolicy &Policy,
                                           unsigned NumRegionInstrs) const {
  Policy.OnlyBottomUp = false;
  Policy.OnlyTopDown = false;
}
}

const llvm::TargetFrameLowering *MOSSubtarget::getFrameLowering() const {
  return &FrameLowering;
}

const llvm::MOSInstrInfo *MOSSubtarget::getInstrInfo() const {
  return &InstrInfo;
}

const llvm::MOSRegisterInfo *MOSSubtarget::getRegisterInfo() const {
  return &InstrInfo.getRegisterInfo();
}

const llvm::MOSSelectionDAGInfo *MOSSubtarget::getSelectionDAGInfo() const {
  return &TSInfo;
}

const llvm::MOSTargetLowering *MOSSubtarget::getTargetLowering() const {
  return &TLInfo;
}

} // namespace llvm

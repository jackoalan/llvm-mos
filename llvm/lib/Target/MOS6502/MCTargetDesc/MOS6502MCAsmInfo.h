#ifndef LLVM_LIB_TARGET_MOS6502_MCTARGETDESC_MOS6502MCASMINFO_H
#define LLVM_LIB_TARGET_MOS6502_MCTARGETDESC_MOS6502MCASMINFO_H

#include "llvm/MC/MCAsmInfo.h"

namespace llvm {

class MOS6502MCAsmInfo : public llvm::MCAsmInfo {
public:
  MOS6502MCAsmInfo();

  bool isAcceptableChar(char C) const override;
  void printEscapedName(raw_ostream& OS, StringRef Name) const override;
};

}  // namespace llvm

#endif  // not LLVM_LIB_TARGET_MOS6502_MCTARGETDESC_MOS6502MCASMINFO_H

; RUN: llc -O0 --filetype=asm < %s | FileCheck %s
target triple = "mos6502"

define i16 @main() {
  ret i16 0
}

; CHECK:      .code
; CHECK:      .global main
; CHECK:      main:
; CHECK:        LDA #0
; CHECK-NEXT:   LDX #0
; CHECK-NEXT:   RTS
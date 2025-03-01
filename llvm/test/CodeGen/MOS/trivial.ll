; RUN: llc -verify-machineinstrs -O0 --filetype=asm < %s | FileCheck %s
target triple = "mos"

define i16 @main() {
  ret i16 0
}

; CHECK:      .text
; CHECK:      .globl main
; CHECK:      main:
; CHECK:        lda #0
; CHECK-NEXT:   ldx #0
; CHECK-NEXT:   rts

; RUN: llc -verify-machineinstrs -o - %s | FileCheck %s
target datalayout = "e-p:16:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-a:8:8-Fi8-n8"
target triple = "mos"

@a.b_c = private unnamed_addr constant [1 x i8] c"H"

define i16 @main() {
; CHECK-LABEL: main:
while.end:
  %0 = getelementptr [1 x i8], [1 x i8]* @a.b_c, i16 0, i16 0
  %1 = ptrtoint i8* %0 to i16
; CHECK:      lda #mos16lo(a.b_c)
; CHECK-NEXT: ldx #mos16hi(a.b_c)
; CHECK-NEXT: rts
  ret i16 %1
}

; CHECK-LABEL: a.b_c:
; CHECK-NEXT: .byte 72

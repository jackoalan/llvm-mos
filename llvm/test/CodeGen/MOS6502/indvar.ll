; RUN: opt -S -indvars %s | FileCheck %s

target datalayout = "e-p:16:8:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-a:8:8-Fi8-n8"
target triple = "mos6502"

@.str = private unnamed_addr constant [15 x i8] c"HELLO, WORLD!\0A\00", align 1

define i16 @main() local_unnamed_addr {
; CHECK-LABEL: define i16 @main()
entry:
  br label %while.body

while.body:
  %0 = phi i8 [ 72, %entry ], [ %2, %while.body ]
  %cur.03 = phi i8* [ getelementptr inbounds ([15 x i8], [15 x i8]* @.str, i8 0, i8 0), %entry ], [ %incdec.ptr, %while.body ]
  %incdec.ptr = getelementptr inbounds i8, i8* %cur.03, i8 1
  %1 = tail call i8 asm sideeffect "", "=r,0"(i8 %0)
  %2 = load i8, i8* %incdec.ptr, align 1
  %tobool.not = icmp eq i8 %2, 0
  ; CHECK: %exitcond = icmp eq i8* %incdec.ptr, getelementptr {{.*}} @.str
  br i1 %tobool.not, label %while.end, label %while.body

while.end:
  ret i16 0
}

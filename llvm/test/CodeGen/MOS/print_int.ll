// RUN: split-file --no-leading-lines %s %t
// RUN: llc -O2 -verify-machineinstrs -o %t/got.s %t/print_int.ll
// RUN: diff --strip-trailing-cr -u %t/want.s %t/got.s

//--- print_int.ll
target datalayout = "e-p:16:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-a:8:8-Fi8-n8"
target triple = "mos"

; Function Attrs: nounwind
define void @print_int(i8 zeroext %x) local_unnamed_addr #0 {
entry:
  %cmp12 = icmp ult i8 %x, 10
  br i1 %cmp12, label %if.then, label %if.end.preheader

if.end.preheader:                                 ; preds = %entry
  %div = udiv i8 %x, 10
  tail call void @print_int(i8 zeroext %div)
  %rem = urem i8 %x, 10
  br label %if.then

if.then:                                          ; preds = %if.end.preheader, %entry
  %x.tr.lcssa = phi i8 [ %x, %entry ], [ %rem, %if.end.preheader ]
  %add = add nuw nsw i8 %x.tr.lcssa, 48
  %0 = tail call i8 asm sideeffect "JSR\09$$FFD2", "=a,0"(i8 %add) #1, !srcloc !2
  ret void
}

attributes #0 = { nounwind "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 12.0.0 (git@github.com:mysterymath/clang6502.git 51e3618d42bc67892d46290a51ea57ea7e127aa6)"}
!2 = !{i32 70}

//--- want.s
	.text
	.file	"print_int.ll"
	.globl	print_int                       ; -- Begin function print_int
	.type	print_int,@function
print_int:                              ; @print_int
; %bb.0:                                ; %entry
	cmp	#10
	bmi	LBB0_2
LBB0_1:                                 ; %if.end.preheader
	sta	_SaveA
	lda	__rc4
	pha
	lda	_SaveA
	sta	__rc4
	ldx	#10
	jsr	__udivqi3
	jsr	print_int
	lda	__rc4
	ldx	#10
	jsr	__umodqi3
	sta	_SaveA
	pla
	sta	__rc4
	lda	_SaveA
LBB0_2:                                 ; %if.then
	clc
	adc	#48
	;APP
	jsr	65490
	;NO_APP
	rts
.Lfunc_end0:
	.size	print_int, .Lfunc_end0-print_int
                                        ; -- End function

; RUN: llc -o - %s | FileCheck %s

target triple = "mos6502"
@a.b_c = private constant i8 2

; CHECK: a_2Eb__c:
; CHECK: .byt 2

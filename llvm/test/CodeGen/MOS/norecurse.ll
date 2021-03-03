; RUN: opt -mos-norecurse -verify -S %s | FileCheck %s
target datalayout = "e-p:16:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-a:8:8-Fi8-n8"
target triple = "mos"

declare void @external_mayrecurse()
declare void @external_nocallback() nocallback

define void @recurses_directly() {
; CHECK: define void @recurses_directly() {
  call void @recurses_directly()
  ret void
}

define void @recurses_indirectlya() {
; CHECK: define void @recurses_indirectlya() {
  call void @recurses_indirectlyb()
  ret void
}

define void @recurses_indirectlyb() {
; CHECK: define void @recurses_indirectlyb() {
  call void @recurses_indirectlya()
  ret void
}

define void @may_recurse_external_call() {
; CHECK: define void @may_recurse_external_call() {
  call void @external_mayrecurse()
  ret void
}

define void @may_recurse_inline_asm() {
; CHECK: define void @may_recurse_inline_asm() {
  call void asm "", ""()
  ret void
}

define void @no_recurse_nocallback() {
; CHECK: define void @no_recurse_nocallback() #1 {
  call void @external_nocallback()
  ret void
}

define void @no_recurse_inline_asm_nocallback() {
; CHECK: define void @no_recurse_inline_asm_nocallback() #1 {
  call void asm "", ""() nocallback
  ret void
}


define void @no_recurse_once_removed() {
; CHECK: define void @no_recurse_once_removed() #1 {
  call void @no_recurse_nocallback()
  ret void
}

; CHECK: #1 = { norecurse }
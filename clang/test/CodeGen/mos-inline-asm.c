// RUN: %clang_cc1 -triple mos -O2 -emit-llvm %s -o - | FileCheck %s

// Test MOSs inline assembly.

char c;

void test_a() {
  // CHECK-LABEL: define void @test_a() {{.*}} {
  // CHECK: [[V:%[0-9]+]] = load i8, i8* @c
  // CHECK: call void asm sideeffect "", "a"(i8 [[V]])
  asm volatile("" :: "a"(c));
}

void test_x() {
  // CHECK-LABEL: define void @test_x() {{.*}} {
  // CHECK: [[V:%[0-9]+]] = load i8, i8* @c
  // CHECK: call void asm sideeffect "", "x"(i8 [[V]])
  asm volatile("" :: "x"(c));
}

void test_y() {
  // CHECK-LABEL: define void @test_y() {{.*}} {
  // CHECK: [[V:%[0-9]+]] = load i8, i8* @c
  // CHECK: call void asm sideeffect "", "y"(i8 [[V]])
  asm volatile("" :: "y"(c));
}


void test_leaf_asm() {
  // CHECK-LABEL: define void @test_leaf_asm() {{.*}} {
  // CHECK: call void asm sideeffect "", ""() #2
  // CHECK: #2 = { nocallback nounwind }
  __attribute__((leaf)) asm volatile("");
}

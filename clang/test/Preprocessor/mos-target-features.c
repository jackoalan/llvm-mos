// RUN: %clang -target mos -mcpu=mos6502 -x c -E -dM %s -o - | FileCheck -match-full-lines --check-prefix=CHECK-6502 %s
// CHECK-6502: #define __mos6502__ 1
// CHECK-6502: #define __mos__ 1

// RUN: %clang -target mos -mcpu=mos6502x -x c -E -dM %s -o - | FileCheck -match-full-lines --check-prefix=CHECK-6502X %s
// CHECK-6502X: #define __mos6502x__ 1
// CHECK-6502X: #define __mos__ 1

// RUN: %clang -target mos -mcpu=mos65c02 -x c -E -dM %s -o - | FileCheck -match-full-lines --check-prefix=CHECK-65C02 %s
// CHECK-65C02: #define __mos65c02__ 1
// CHECK-65C02: #define __mos__ 1

// RUN: %clang -target mos -mcpu=mosr65c02 -x c -E -dM %s -o - | FileCheck -match-full-lines --check-prefix=CHECK-R65C02 %s
// CHECK-R65C02: #define __mos__ 1
// CHECK-R65C02: #define __mosr65c02__ 1

// RUN: %clang -target mos -mcpu=mosw65c02 -x c -E -dM %s -o - | FileCheck -match-full-lines --check-prefix=CHECK-W65C02 %s
// CHECK-W65C02: #define __mos__ 1
// CHECK-W65C02: #define __mosw65c02__ 1

// RUN: %clang -target mos -mcpu=mosw65816 -x c -E -dM %s -o - | FileCheck -match-full-lines --check-prefix=CHECK-W65816 %s
// CHECK-W65816: #define __mos__ 1
// CHECK-W65816: #define __mosw65816__ 1

// RUN: %clang -target mos -mcpu=mosw65el02 -x c -E -dM %s -o - | FileCheck -match-full-lines --check-prefix=CHECK-65EL02 %s
// CHECK-65EL02: #define __mos__ 1
// CHECK-65EL02: #define __mosw65el02__ 1

// RUN: %clang -target mos -mcpu=mosw65ce02 -x c -E -dM %s -o - | FileCheck -match-full-lines --check-prefix=CHECK-65CE02 %s
// CHECK-65CE02: #define __mos__ 1
// CHECK-65CE02: #define __mosw65ce02__ 1

// RUN: %clang -target mos -mcpu=mossweet16 -x c -E -dM %s -o - | FileCheck -match-full-lines --check-prefix=CHECK-SWEET16 %s
// CHECK-SWEET16: #define __mos__ 1
// CHECK-SWEET16: #define __mossweet16__ 1

// RUN: %clang -target mos -mcpu=mosspc700 -x c -E -dM %s -o - | FileCheck -match-full-lines --check-prefix=CHECK-SPC700 %s
// CHECK-SPC700: #define __mos__ 1
// CHECK-SPC700: #define __mosspc700__ 1

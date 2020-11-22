# The clang6502 Compiler

This project will port the Clang frontend and LLVM backend to the 6502.
Initially, code will be generated for the ca65 assembler, so that the existing
cc65 ecosystem can be used (to a degree). This will be a barebones, baremetal
implementation until the compiler produces very, very good assembly.

## Project Status

The compiler can do "return 0" and a simple C64 KERNAL JSR hello world, and
that's about it. Stay tuned; implementation is progressing at a steady clip.

Updated November 21, 2020.


# The clang6502 Compiler

This project will port the Clang frontend and LLVM backend to the 6502.
Initially, code will be generated for the ca65 assembler, so that the existing
cc65 ecosystem can be used (to a degree). This will be a barebones, baremetal
implementation until the compiler produces very, very good assembly.

## Project Status

The compiler can compile a simple "Hello world" program for the C64 in both
speed- and size-optimized modes. See below for examples of the input/output,
as well as the remaining tasks for this case study.

## Hello World

`main.c`:

```C
int main(void) {
	const char *cur = "HELLO, WORLD!\n";
	while (*cur) {
		char c = *cur++;
		asm volatile ("JSR\t$FFD2" : "+a"(c));
	}
}
```

### Speed Optimized (-O2)

`$ clang --target=mos6502 -S -O2 main.c`

```asm
.code
.global	main
main:
	LDA	#72
	LDY	#69
	;APP
	JSR	$FFD2
	;NO_APP
	LDX	#76
	TYA
	;APP
	JSR	$FFD2
	;NO_APP
	TXA
	;APP
	JSR	$FFD2
	;NO_APP
	LDY	#79
	TXA
	;APP
	JSR	$FFD2
	;NO_APP
	TYA
	;APP
	JSR	$FFD2
	;NO_APP
	LDA	#44
	;APP
	JSR	$FFD2
	;NO_APP
	LDA	#32
	;APP
	JSR	$FFD2
	;NO_APP
	LDA	#87
	;APP
	JSR	$FFD2
	;NO_APP
	TYA
	;APP
	JSR	$FFD2
	;NO_APP
	LDA	#82
	;APP
	JSR	$FFD2
	;NO_APP
	LDY	#68
	TXA
	;APP
	JSR	$FFD2
	;NO_APP
	LDX	#33
	TYA
	;APP
	JSR	$FFD2
	;NO_APP
	LDY	#10
	TXA
	;APP
	JSR	$FFD2
	;NO_APP
	TYA
	;APP
	JSR	$FFD2
	;NO_APP
	LDA	#0
	LDX	#0
	RTS
```

Notes:
  - The loop is fully unrolled, as expected for a small constant number of
    iterations.
  - The L and O characters are placed in registers, since a transfer is cheaper than
    an immediate load, and these letters are used twice.

TODO:
  - For some reason, the 'E' is placed in Y, then moved to A, when it would be
    more efficient to load it directly to A, since the letter is only used once.

### Size Optimized (-Os)

`$ clang --target=mos6502 -S -Os main.c`

```asm
.code
.global	main
main:
	LDA	#<_2Estr
	LDY	#>_2Estr
	STA	z:__ZP__0
	LDX	#72
	STY	z:__ZP__1
	LDY	#0
LBB0__1:
	CLC
	LDA	z:__ZP__0
	ADC	#1
	STA	z:__ZP__0
	LDA	z:__ZP__1
	ADC	#0
	STA	z:__ZP__1
	TXA
	;APP
	JSR	$FFD2
	;NO_APP
	LDA	(__ZP__PTR__0),Y
	TAX
	CPX	#0
	BNE	LBB0__1
	JMP	LBB0__2
LBB0__2:
	LDA	#0
	LDX	#0
	RTS

.rodata
_2Estr:
	.byt	72,69,76,76,79,44,32,87,79,82,76,68,33,10,0

.zeropage
__ZP__PTR__0:
	.res	2

__ZP__0 = __ZP__PTR__0
__ZP__1 = __ZP__0+1
```

Notes:
  - The loop was rotated so there's only one branch per iteration.
  - Fake ZP registers are allocated by LLVM's register allocator, then these
    registers are lowered to real zero page segment addresses in the backend.
  - Aside from specific pairs, the layout of the zero page segment is
    intentionally unconstrained.

TODO:
  - INC and branching can more efficiently increment ZP than ADC 1, but this
    requires basic block manipulation.
  - The CPX 0 is redundant; a pass should eliminate redundant compares.
  - The JMP is redundant; a pass should eliminate redundant jumps to fallthrough.
  - Branch relaxation is not yet implemented, so the branches could be out of range.
  - The constant #0 is already available in Y, so it can be TYA and TAX to
    produce the return value.
  - Because the pointer is known not to overflow 64k (undefined behavior), the
    carry is known clear after the final add. Thus, the CLC can be hoisted out
    of the loop. This optimization may be difficult, since heavy loop
    manipulation would need to be done at the machine code level.
  - LLVM does not rewrite the loop induction variable to be the pointer, since
    the pointer is 16-bits, which is not a native integer type. Custom induction
    variable rewriting could use the 8-bit offset as the induction variable
    instead, which would allow selecting an indirect addressing mode for the
    pointer. This may not always pay off, though, so more thought is needed.

Updated December 14, 2020.


# The clang6502 Compiler

This project will port the Clang frontend and LLVM backend to the 6502.
Initially, code will be generated for the ca65 assembler, so that the existing
cc65 ecosystem can be used (to a degree). This will be a barebones, baremetal
implementation until the compiler produces very, very good assembly.

## Project Status

The compiler can compile a few simple example programs for the C64 in both
speed- and size-optimized modes. The compiler is intentionally not very general
at the moment: stray slightly from the examples, and you'll almost certainly
encounter "not yet implemented" errors.

## Milestones

### De-risk

The compiler is currently being "de-risked." Any aspects of efficient code
generation where solutions may require significant rewrites later are being
handled first. The goal of this phase is to do as many rewrites as possible
up-front, while the codebase is small.

### Presumed C99 Compatibility

Once the overall architecture starts to settle down, the project will march
towards presumed C99 compatibility as quickly as possible. The quality of
generated code will be pretty bad in a lot of cases, but it should always
be clear how to make it better without a complete rewrite. Otherwise, the
project will return to the derisk phase.

### Passing C99 LLVM Test Suite

At this point, minimum library support needed for the LLVM test suite will be written.
Some 6502 simulator will need to be chosen, and the test suite run against it. Any
discovered issues preventing C99 compatibility will be fixed. Only tests relevant
for a freestanding implementation will be run.

### Optimization

At this point, optimization opportunities will be implemented, from greatest
to smallest impact. Once the big obvious opportunities are exhausted, LLVM's
benchmark suite will be used to prioritize and implement minor optimizations.

## Calling convention

- Non-pointer arguments/return values are passed in `A`, then `X`, then `Y`, then
  each available ZP register, from `ZP_0` to `ZP_253`.

- Pointer arguments/return values are passed in successively increasing pairs
  of ZP locations (ZP pointer regisers).

- Once the above locations are exhausted, the remainder are placed on the
  soft stack.

- Aggregate types (structs, arrays, etc.) are passed by pointer. The pointer
  is managed entirely by the caller, and may or may not be on the soft stack.
  The callee is free to write to the memory; the caller must consider the
  memory overwritten by the call.

- Aggregate types are returned by a pointer passed as an implicit first
  argument. The resulting function returns void.

- ZP_PTR_1 and ZP_PTR_3 (and subregisters) are callee-saved. All other
  ZP locations, registers, and flags are caller-saved.

- Variable arguments (those within the ellipses of the argument list) are
  passed through the stack. Named arguments before the variable arguments are
  passed as usual: first in registers, then stack. Note that the variable
  argument and regular calling convention differ; thus, variable argument
  functions must only be called if prototyped. The C standard requires this,
  but many platforms do not; their variable argument and regular calling
  conventions are identical. A notable exception is Apple ARM64.

## Stack

The C stack is coming along:

- The 6502 hard stack and a virtual 16-bit soft stack are together used as the C stack.

- To avoid running out of room, only 4 bytes worth of hard stack are allowed per frame.

- LLVM's stack slot coloring places the most important values in hard stack.

- A whole-program IPO pass detects whether or not functions might be recursive.
  - All local stack locations of nonrecursie functions are lowered to static
    memory locations, since at most one invocation can be active at a time.
  - Nonrecursive callers may still end up placing outgoing arguments on the
    soft stack, since the callee may or may not be recursive. Recursiveness is
    not part of the ABI.

- Variable sized stack frames (`alloca`) are not yet supported.

Eventually, local variables in recursive functions that do not live across
possibly-recursive calls can also be lowered to static memory locations. This
will require analyzing the live ranges of the slots.

Very eventually, the static stacks of each function can be merged together
by examining the call graph. Functions that cannot simultaneously be active
can have overlapping static stack locations, as would be the case if a real
hard or soft stack were used.

## Zero page

The number of zero-page memory location can be specified using the
`--num-zp-ptrs` compiler flag. This determines the number of consecutive
two-byte pointer locations the compiler will use when generating code. At least
one such pointer must be available; otherwise, the compiler would be unable to
indirectly access memory without self-modifying code.

The first two zero page locations are `__ZP__0` and `__ZP__1`, the next two (if
available) are `__ZP__2` and `__ZP__3`, and so forth. The compiler makes no
assumptions about the absolute arrangement of these pairs in memory, only that
the two bytes in each pair are consecutive. The compiler presently has no way to
use zero page locations that are neither the high nor the low byte of a pair.

One 2-byte pointer register is always reserved by the compiler as a soft stack;
it's low and high bytes are the external symbols `__SPlo` and `__SPhi`,
respectively. This location is not included in the count given to
`--num-zp-ptrs`.

At the end of code generation, references to the zero page registers are lowered
to abstract symbols to placed in the zero page by the linker. Only registers
actually accessed are emitted. It's up to the C runtime environment to actually
allocate memory behind these symbols; they're considered external to every C
module.

## Memory

The compiler currently requires 6 absolute memory locations for emergency saving
and restoring: `__SaveA`, `__SaveX`, `__SaveY`, `__SaveP`, `__SaveZPlo`, and
`__SaveZPhi`. These locations are considered external to the generated assembly,
and they must be defined somewhere by the C runtime for the generated output to
be correct.

Right now, these locations are used quite naively. A more sophisticated approach
would elide many of them to pushes and pulls, and instruction reordering to
adjust live ranges should eliminate most of the rest. Use of these locations
should become increasingly rare, but it appears there will always be situations
that cannot be completely handled using the hard stack, since generalized push
and pull pseudoinstructions may themselves require saving live values, and
push/pull around the pseudos will always interfere with there operation.

Eventually, these locations can be handled like any other de-stackified memory
locations, as they're guaranteed not to be live across calls. Accordingly, they
can be lowered to stack indices, which can in turn be lowered to global memory
locations, with global coloring to reuse those locations where live ranges don't
interfere. This would remove the requirement for the C runtime to be aware of
these locations at all. This should also allow assigning them to unused zero
page locations, which is fewer cycles than push/pull and nearly as dense.

## Examples

### Char stats (Non-recursive)

Routine that collects counts of number of each character seen on the
Commodore 64. By annotating external routines as "leaf", the compiler can prove
char_stats does not recurse, allowing the array to be allocated statically.
Excersises static stack, array access, and no-recurse detection.

<details>
	<summary>char_lib.h</summary>

```C
__attribute__((leaf)) char next_char();
__attribute__((leaf)) void report_counts(int counts[256]);
```

</details>

<details>
	<summary>char_stats.c</summary>

```C
#include "char_lib.h"

void char_stats() {
	int counts[256] = {0};
	char c;
	while ((c = next_char())) {
		counts[c]++;
	}
	report_counts(counts);
}
```

</details>

<details>
	<summary>Optimized (-O2/-Os)</summary>

`$ clang --target=mos6502 -S -O2 char_stats.c`

```asm
.code
.global	char__stats
char__stats:
	LDX	#0
	LDA	#<char__stats__sstk
	STA	z:__ZP__0
	LDA	#>char__stats__sstk
	STA	z:__ZP__1
	LDA	#0
	LDY	#2
	JSR	memset
LBB0__1:
	ASL	A
	STA	z:__ZP__0
	LDA	#0
	ROL	A
	STA	z:__ZP__1
	LDA	#<char__stats__sstk
	LDX	#>char__stats__sstk
	CLC
	ADC	z:__ZP__0
	STA	z:__ZP__0
	TXA
	ADC	z:__ZP__1
	STA	z:__ZP__1
	LDY	#0
	LDA	(__ZP__0),Y
	CLC
	ADC	#1
	STA	(__ZP__0),Y
	LDY	#1
	LDA	(__ZP__0),Y
	ADC	#0
	STA	(__ZP__0),Y
LBB0__2:
	JSR	next__char
	CMP	#0
	BNE	LBB0__1
LBB0__3:
	LDA	#<char__stats__sstk
	STA	z:__ZP__0
	LDA	#>char__stats__sstk
	STA	z:__ZP__1
	JSR	report__counts
	RTS

.bss
char__stats__sstk:
	.res	512

.global	__ZP__0
.global	__ZP__1
.global	memset
.global	next__char
.global	report__counts
```

Notes:

  - The leaf attribute annotations tell the compiler that the external routines
    cannot recursively call any external routines in the current module. This
    will be typical of C library functions. Such annotations are only required
    for external symbols; the control flow of function definitions can be
    examined directly otherwise.

  - Accordingly, this allows the compiler to prove that `char_stats` is not
    recursive. No annotation need be made on this function itself.

  - Because the function cannot recurse, only one invocation could be active at
    a time. Thus, all its local variables can be allocated to a static memory
    region. In this case, the region is `__char_stats__stk`.

  - The stack pointer does not need to be adjusted, since no soft or hard stack
    is used.

</details>

### Char Stats

Routine that collects counts of number of each character seen on the
Commodore 64. Excersises soft stack and array access.

<details>
	<summary>char_stats.c</summary>

```C
char next_char();
void report_counts(int counts[256]);

void char_stats() {
	int counts[256] = {0};
	char c;
	while ((c = next_char())) {
		counts[c]++;
	}
	report_counts(counts);
}
```

</details>

<details>
	<summary>Optimized (-O2/-Os)</summary>

`$ clang --target=mos6502 -S -O2 char_stats.c`

```asm
.code
.global	char__stats
char__stats:
	CLC
	LDA	#254
	ADC	z:__SPhi
	STA	z:__SPhi
	LDX	#0
	LDA	z:__SPlo
	STA	z:__ZP__0
	LDA	z:__SPhi
	STA	z:__ZP__1
	LDA	#0
	LDY	#2
	JSR	memset
LBB0__1:
	ASL	A
	STA	z:__ZP__0
	LDA	#0
	ROL	A
	STA	z:__ZP__1
	LDA	z:__SPlo
	LDX	z:__SPhi
	CLC
	ADC	z:__ZP__0
	STA	z:__ZP__0
	TXA
	ADC	z:__ZP__1
	STA	z:__ZP__1
	LDY	#0
	LDA	(__ZP__0),Y
	CLC
	ADC	#1
	STA	(__ZP__0),Y
	LDY	#1
	LDA	(__ZP__0),Y
	ADC	#0
	STA	(__ZP__0),Y
LBB0__2:
	JSR	next__char
	CMP	#0
	BNE	LBB0__1
	LDA	z:__SPlo
	STA	z:__ZP__0
	LDA	z:__SPhi
	STA	z:__ZP__1
	JSR	report__counts
	CLC
	LDA	#2
	ADC	z:__SPhi
	STA	z:__SPhi
	RTS

.global	__SPhi
.global	__SPlo
.global	__ZP__0
.global	__ZP__1
.global	memset
.global	next__char
.global	report__counts
```

Notes:

  - Setting up the stack frame only adjusts the high byte of `__SP`, since the
    frame is a multiple of 256 bytes.
  - The compiler uses memset to clear the counts array.
  - Since next_char and report_counts are both external, the compiler cannot be
    certain that they do not call char_stats recursively, forcing the use of the
    C stack instead of static memory.
  - The character retrieved from next_char is shifted left to form the array
    offset, with the low byte in `__ZP__0` and the high byte in `__ZP__1`.
  - The rotations are done in A, then transferred to ZP. If the values both in and out
    were in ZP, the rotations would have been done in ZP instead.
  - The offset is added to the array start to form the count address.
  - The read-modify-write operation to increment the count is threaded one byte
    at a time through `LDA` `ADC` `STA`, since neither the load nor the store
    clobber the carry bit. Otherwise, the both the high and low bytes of the
    load would have scheduled first, then both bytes of the increment, then both
    bytes of the store. This bit of magic is brought to you by LLVM's machine
    scheduler, which endeavors to keep the register pressure as low as possible
    by reordering instructions prior to register allocation.
  - Direct stack pointer access optimization:
    - Instead of reading `__SPlo` and `__SPhi` into `A` and `X`, they could be
    left where they are, and the low and high bytes of the offset could then be
    placed there instead. This would allow adding them to the stack pointer
    directly, saving a pair of stores.
    - Unfortunately, it's unlikely I'll be able to get LLVM to do this. The fact
    that the array offset is exactly equal to the stack pointer is only known
    once the stack layout has been finalized, which in turn can only be done
    after register allocation, since the register allocator may spill values to
    the stack. But after register allocation the offset is already assigned to
    `A` and `X`, and the allocation would essentially need to be re-done at this
    point, which could itself spill, leading to an infinite regress. This is a
    classic sort of "phase ordering" problem endemic to the heuristic approach
    used by compilers that separate instruction selection, frame lowering, and
    register allocation, as does LLVM. The tradeoff is that solving the combined
    problem efficiently is tremendously harder, and the available heuristics for
    the combined problem tend to work much worse than the heuristics for the
    each of the separated problems.

TODO:

  - `LDY #1` can be replaced with `INY`, saving a byte.

</details>

### Hello World

Outputs "HELLO, WORLD" on a Commodore 64. Excersises trivial code generation,
inline assembly, and loop strength reduction.

<details>
	<summary>main.c</summary>

```C
int main(void) {
	const char *cur = "HELLO, WORLD!\n";
	while (*cur) {
		char c = *cur++;
		asm volatile ("JSR\t$FFD2" : "+a"(c));
	}
}
```

</details>

<details>
	<summary>Size optimized (-Os)</summary>

`$ clang --target=mos6502 -S -Os main.c`

```asm
.code
.global	main
main:
	LDX	#0
	LDA	#72
LBB0__1:
	;APP
	JSR	$FFD2
	;NO_APP
	LDA	_2Estr+1,X
	INX
	CPX	#14
	BNE	LBB0__1
	LDA	#0
	LDX	#0
	RTS

.rodata
_2Estr:
	.byt	72,69,76,76,79,44,32,87,79,82,76,68,33,10,0
```

Notes:
- The loop was rotated so there's only one branch per iteration.
- The string offset was statically determined to fit within an unsigned 8-bit
integer, allowing indexed addressing mode for the load.

TODO:

- LDX #0 immediately follows LDA #0, when it would be more efficient to TAX.

</details>

<details>
	<summary>Speed optimized (-O2)</summary>

`$ clang --target=mos6502 -S -O2 main.c`

```asm
.code
.global	main
main:
	LDA	#72
	;APP
	JSR	$FFD2
	;NO_APP
	LDA	#69
	;APP
	JSR	$FFD2
	;NO_APP
	LDX	#76
	TXA
	;APP
	JSR	$FFD2
	;NO_APP
	TXA
	;APP
	JSR	$FFD2
	;NO_APP
	LDY	#79
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
	TXA
	;APP
	JSR	$FFD2
	;NO_APP
	LDA	#68
	;APP
	JSR	$FFD2
	;NO_APP
	LDA	#33
	;APP
	JSR	$FFD2
	;NO_APP
	LDA	#10
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
    
</details>

### Print Int

Routine that prints an unsigned integer on the Commodore 64. Excersises
libcalls, stack usage, and recursion.

<details>
	<summary>print_int.c</summary>

```C
void print_int(char x) {
	if (x < 10) {
		x += '0';
		asm volatile ("JSR\t$FFD2" : "+a"(x));
		return;
	}
	print_int(x / 10);
	print_int(x % 10);
}
```

</details>

<details>
	<summary>Optimized (-O2/-Os)</summary>

`$ clang --target=mos6502 -S -O2 print_int.c`

```asm
.code
.global	print__int                      ; -- Begin function print_int
print__int:                             ; @print_int
; %bb.0:                                ; %entry
	CMP	#10
	BMI	LBB0__2
; %bb.1:                                ; %if.end.preheader
	PHA                                     ; 1-byte Folded Spill
	LDX	#10
	JSR	____udivqi3
	JSR	print__int
	LDX	#10
    TSX
    LDA 257,X
	JSR	____umodqi3
	PLA
LBB0__2:                                ; %if.then
	CLC
	ADC	#48
	;APP
	JSR	$FFD2
	;NO_APP
	RTS
                                        ; -- End function
.global	____udivqi3
.global	____umodqi3
```

Notes:

- `__udivqi3` and `__umodqi3` are external routines that provide unsigned
  integer division and modulus. As per expected calling convention, the operands
  are taken in A and X, and the result is returned in A.
- LLVM notices that the second recursive call, `print_int(x % 10)`, will always
  have `x < 10`, so it inlines that outcome, saving a whole call. The inlined
  code is shared with the if branch of the outer call, so no additional code is
  required either.
- A value needs to be saved across the calls to `__udivqi3` and `print__int`. A
  `PHA` prolog and `PLA` epilog increase and decrease the size of the hard
  stack, and the indexed addressing mode is used to save and restore the value
  to the top hard stack location. The indexed addressing save/restore
  instructions are optimized away in this example, so only the `PHA` and `PLA`
  are present. (I.e., the load and store are folded into these instructions.)
- The prolog and epilog are shrink-wrapped to the only basic block with stack
  operations, so they aren't executed in the `BMI` path. This also aids with
  folding the save/restore instructions, since it places the prologue and
  epilogue in the same basic block as the instructions it can be folded with.

TODO:

- Division/remainder by constant can be expanded to a multiplication of the
  inverse, which is far more efficient. LLVM's older SelectionDAG framework does
  this transformation, but GlobalISel (used by this target) doesn't yet. Nearly
  every target needs this optimization, so I'll wait on implementing this until
  the end; they may get around to it before then.
- A `__udivmodqi4` instruction would be twice as efficient as calculating the
  division twice, but would require either struct return or pointer argument.
  Neither of which is currently implemented. GlobalISel should really do this
  too for many targets; as with the above, I'll wait till the end to implement
  this just in case LLVM devs get to it first.

</details>

Updated February 17, 2021.

# The clang6502 Compiler

This project will port the Clang frontend and LLVM backend to the 6502.
Initially, code will be generated for the ca65 assembler, so that the existing
cc65 ecosystem can be used (to a degree). This will be a barebones, baremetal
implementation until the compiler produces very, very good assembly.

## Project Status

The compiler can compile a few simple example programs for the C64 in both
speed- and size-optimized modes. The compiler is intentionally not very general
at the moment: stray slightly from the examples, and you'll almost certainly
encounter "not yet implemented" errors. The examples/milestones are chosen to
exercise different aspects of code generation, with riskier aspects first.
Once the remaining tasks are more or less mechanical, the compiler will be known
to have "good bones." At that point, work will move from section to section,
generalizing each and filling out the compiler until it reaches MVP.

## Generated code characteristics

### Calling convention

The calling convention is presently very barebones:
- Only 8/16-bit integers are allowed.
- Arguments are passed in A, then X, then Y.
- The return value is passed in A, then X.
- The compiler bails if the arguments/return value don't fit.
- All compiler-used ZP locations, all registers, and all flags are caller-saved.
  - Use of most physical registers/flags are mandatory for certain operations, so functions would be forced to save/restore them if they were callee saved,
    even if they held values not live across calls. This should be the usual case: since there are so few, live ranges should be short.
  - TODO: Some ZP locations should be callee-saved; that way the compiler can amortize save/restore cost of different values to the prolog/epilog. A single
    callee saved reg may be able to keep a dozen different values live across calls if their live ranges don't interfere, all for the cost of one `PHA;PLA`.
    Doing this with caller-saved regs would in the worst cast require one `PHA/PLA` per call site.

### Stack usage

The C stack is presently very barebones:
- The 6502 hard stack is used as the C stack
- To avoid running out of room, only 4 bytes worth of stack are allowed per frame.
- Variable sized stack frames (`alloca`) are not yet supported.

Eventually, a cc65-style soft stack will be added to the 4-byte hard stack frame, for large or infrequently accessed objects. The hard stack will be used as much as possible for the most important values (that don't fit into ZP registers).

Very eventually, a whole-program IPO pass will detect when stack frames are used in procedures that are never recursive (directly or indirectly). Any stack references in such locations will be lowered to globally static memory locations. Such locations will be globally colored: any two "static stack frames" that cannot be simultaneously active can share the same color, that is, they may overlap in memory.

### Zero page usage

The compiler currently assumes it has access to 128 2-byte zero page pointer registers, each composed of 2 individually addressable zero page registers.
The compiler makes no assumptions about the relative layout of these registers, and at the end of code generation, references to the zero page registers
are lowered to abstract symbols placed in the zero page by the linker. Only registers actually accessed are emitted.

One 2-byte pointer register is presently reserved by the compiler for saving/restoring. Accoringly, at least two zero page registers must be available
for compiler use.

Eventually, a compiler flag should allow the user to specify how many zero page locations are available for use by the compiler. This will limit the register allocator and potentially increase the amount of stack used by the generated code.

## Examples

### Hello World

Outputs "HELLO, WORLD" on a Commodore 64. Excersises trivial code generation, inline assembly, and loop strength reduction.

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

Routine that prints an unsigned integer on the Commodore 64. Excersises libcalls, stack usage, and recursion.

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
.global	print__int
print__int:
	PHA
	CMP	#10
	BMI	LBB0__2
	LDX	#10
	STX	z:__ZP__0
	TSX
	STA	257,X
	LDX	z:__ZP__0
	JSR	____udivqi3
	JSR	print__int
	TSX
	LDA	257,X
	LDX	#10
	JSR	____umodqi3
LBB0__2:
	CLC
	ADC	#48
	;APP
	JSR	$FFD2
	;NO_APP
	PLA
	RTS

.zeropage
__ZP__0:
	.res	1

.global	____udivqi3
.global	____umodqi3
```

Notes:

- `__udivqi3` and `__umodqi3` are external routines that provide unsigned integer division and modulus. As per expected calling convention,
  the operands are taken in A and X, and the result is returned in A.
- LLVM notices that the second recursive call, `print_int(x % 10)`, will always have `x < 10`, so it inlines that outcome, saving a whole call.
  The inlined code is shared with the if branch of the outer call, so no additional code is required either.
- A value needs to be saved across the calls to `__udivqi3` and `print__int`. A `PHA` prolog and `PLA` epilog increase and decrease the size of the hard stack,
  and the indexed addressing mode is used to save and restore the value to the top hard stack location. This is clumsy, but general.

TODO:

- The means by which values are loaded/stored with the hard stack can be considerably improved, but the biggest improvement would be to combine the epilog
  and prolog with the load and store. A `PHA` is a combined prolog and store, and a `PLA` is a combined epilog and load, and the rest of the save/restore
  logic would then disappear. This transformation isn't always safe: if different control flow paths see different numbers of `PHA` operations, then different
  offsets would be required to perform indexed addressing, depending on how the block containing the indexed address mode was reached. Still, making use of the
  PHA/PLA instructions saves a huge number of instructions, so the optimization is very important to get right.  
- A `__udivmodqi4` instruction would be twice as efficient as calculating the division twice, but would require either struct return or pointer argument.
  Neither of which is currently implemented.

</details>

Updated December 29, 2020.

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

## Risk Factors

<dl>
  <dt>Restricted zero page register set</dt>
  <dd>
    Can the compiler be made to use fewer zero page registers when requested via
    command line flag? The risk here is that the target description will become
    completely unmanageable, and/or TableGen will explode.
  </dd>
	
  <dt>Far stack access</dt>
  <dd>
    Can the compiler access locals that are more than 256 bytes above the stack pointer?
    How about parameters? The only available indirect addressing mode has at most 256 byte
    offsets, so virtual frame registers are required for both. Can these be scavenged if
    necessary? How do we avoid redundant calculation of these frame registers?
  </dd>
  
  <dt>Stack frame elision</dt>
  <dd>
    Can non-recursive functions be detected reliably enough for stack frame
    elision? Can inline assembly and external calls be annotated to prevent
    them from appearing to possibly call main()? To what degree does link-time
    optimization decrease the burden on this analysis?
  </dd>
  
  <dt>PostRA pseudo scheduling</dt>
  <dd>
    Post-register-allocation pseudo-instructions cannot report their full side-effect profile, since
    their implementation will wildly differ depending on where the register allocator places their
    inputs and outputs. Once register allocation occurs, their side effect profiles will be known,
    and they might be reschedulable to locations where they don't interfere with live registers.
    It's unlikely that we'd be able to rely on the existing scheduler for this though, since the pseudos
    will behave as advertised by emitting save/restore code. At no point do they exhibit their real
    side-effect profile.
  </dd>
  
  <dt>Parameter stack elision</dt>
  <dd>
    Can stack for incoming parameters be elided for nonrecursive functions?
    Ideally, the caller of such a function would place arguments directly into
    the static stack frame of the callee. However, this would make non-recursion
    part of the ABI of the function, so this would likely only be possible for
    internal functions. If the parameters cannot be elided, can the locals still be?
    How about outgoing parameters to recursive callees?
  </dd>
  
  <dt>PostRA pseudo scavenging</dt>
  <dd>
    PostRA pseudo save/restore logic tightly wraps the affected region. If the register scavenger were
    used instead, then live registers could be spilled around broad regions, preventing redundant saves
    and restores from ever being emitted.
  </dd>
  
  <dt>Multibyte operations</dt>
  <dd>
    Beyond simple ADC, multibyte operations on the 6502 can look drastically different
    depending on how they are implemented. For example, >> 8 a 16-bit int
    by 8 would be a simple copy of the high byte to the low byte and a clear of the high byte,
    while shifting it by one would be a pair of LSR/ROR. Multibyte comparisons can work either
    from the high byte to the low byte, or from the low byte to the high byte. The efficiencies
    of these alternatives need to be studied, and enough examples constructed to suggest that
    it's possible to fill in the rest.
  </dd>
</dl>

## Current Working Example

Code generation for the following example is currently being tuned. Once the
assembly quality is acceptable, this section will be updated with a new example
that exercises different aspects of code generation.

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
	LDA	#0
	LDY	#2
	LDX	z:__SPlo
	STX	z:__ZP__0
	LDX	z:__SPhi
	STX	z:__ZP__1
	TAX
	JSR	memset
	JMP	LBB0__2
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

## Generated code characteristics

### Calling convention

The calling convention is presently very barebones:
- Non-pointer arguments/return values are passed in `A`, then `X`, then `Y`.
- Pointer arguments/return values are passed in successively increasing pairs
  of ZP locations.
- The compiler bails if the arguments/return value don't fit.
- All compiler-used ZP locations, all registers, and all flags are caller-saved.
  - Use of most physical registers/flags are mandatory for certain operations,
    so functions would be forced to save/restore them if they were callee saved,
    even if they held values not live across calls. This should be the usual
    case: since there are so few, live ranges should be short.
  - TODO: Up to four ZP locations should be callee-saved; that way the compiler
    can amortize save/restore cost of different values to the prolog/epilog. A
    single callee saved reg may be able to keep a dozen different values live
    across calls if their live ranges don't interfere, all for the cost of one
    `PHA;PLA`. Doing this with caller-saved regs would in the worst cast require
    one `PHA/PLA` per call site.
    - It turns out doing this will require some hefty modfifications to LLVM's
      greedy register allocator. The allocator *really* likes registers; it'll
      prefer a CSR over spilling unless one of two very specific conditions are
      met. For the 6502, a spill/reload may just be a `PHA/PLA` pair, while
      saving a CSR would require from : `LDA csr; PHA; ...; PLA; STA csr`, to,
      if A and NZ need to be saved: `PHP; PHA; LDA csr; PHA; ...; PLA; PLA; STA
      csr; PLP`. This also doesn't count additional cost if the CSR needs to be
      loaded to a GPR for use. Thus using CSR's should be situational; regalloc
      should evaluate after the fact of whether using a CSR was worth it, and if
      not, emit spills for all values allocated to the CSR.

### Stack usage

The C stack is coming along:
- The 6502 hard stack and a virtual 16-bit soft stack are together used as the C stack.
- To avoid running out of room, only 4 bytes worth of hard stack are allowed per frame.
- LLVM's stack slot coloring places the most important values in hard stack.
- Variable sized stack frames (`alloca`) are not yet supported.

Very eventually, a whole-program IPO pass will detect which stack indices are
live across any calls that might be recursive (either directly or indirectly).
Only these stack indices need to be stored on an actual stack; all others will
be lowered to globally static memory locations. Such locations will be globally
colored: any two "static stack frames" that cannot be simultaneously active can
share the same color, that is, they may overlap in memory. This combines the
best qualities of static and stack allocation: a pre-allocated block will be
presented to the linker, and that block will be no larger than the largest
possible non-recursive call stack that the program could ever achieve at
runtime.

### Zero page usage

The compiler currently assumes it has access to 128 2-byte zero page pointer
registers, each composed of 2 individually addressable zero page registers. The
compiler makes no assumptions about the relative layout of these registers, and
at the end of code generation, references to the zero page registers are lowered
to abstract symbols placed in the zero page by the linker. Only registers
actually accessed are emitted.

One 2-byte pointer register is presently reserved by the compiler as a soft
stack; it's low and high bytes are the external symbols `__SPlo` and `__SPhi`,
respectively.

Eventually, a compiler flag should allow the user to specify how many zero page
locations are available for use by the compiler. This will limit the register
allocator and potentially increase the amount of stack used by the generated
code.

### Memory usage

The compiler currently requires 4 absolute memory locations for emergency saving
and restoring: `__SaveA`, `__SaveX`, `__SaveY`, and `__SaveP`. These locations
are considered external to the generated assembly, and they must be defined
somewhere by the C runtime for the generated output to be correct.

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

## Further Examples

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
	LDX	#10
	PHA                                     ; 1-byte Folded Spill
	JSR	____udivqi3
	JSR	print__int
	PLA                                     ; 1-byte Folded Reload
	LDX	#10
	JSR	____umodqi3
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

Updated January 24, 2021.

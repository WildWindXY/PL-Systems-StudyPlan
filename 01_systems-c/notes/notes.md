# Phase 1: Systems Foundations — C-level Intuition

## Day 1: C Representation & Toolchain

### Reading Notes: CS:APP 2.1–2.4

> **Definition.** *Word size* is the nominal size of pointer data.  
> **Example**: \( 32\text{-bit} \) word size limits virtual address space to \( 2^{32} \text{ bytes} \approx 4\,\text{GB} \).

> **Definition.** *ABI* (Application Binary Interface) defines how binary programs interface with the system and each other at the machine level.
> 
> **Key Components of ABI:**
> - **Data Layout**: Size, alignment, and layout of primitive types (e.g., `int`, `long`, `struct`).
> - **Calling Convention**: Rules for passing arguments, return values, and using registers.
> - **System Call Interface**: How user programs invoke OS kernel services.
> - **Linking and Symbol Resolution**: How functions and variables are resolved across binaries.

> **Boolean Algebra.**
> $$
> \begin{aligned}
> \text{AND:} & \quad x \& y \\
> \text{OR:} & \quad x | y \\
> \text{XOR:} & \quad x \wedge y \\
> \text{NOT:} & \quad \sim x
> \end{aligned}
> $$

> **Shift Operation.**
>  - **Logical shift**: Fills with `0`, used for **unsigned** data
>  - **Arithmetic shift**: Fills with **sign bit**, used for **signed** data

> **Signed Integer Representations:** Two's Complement

> **Floating-Point Representation:**
> - Normalized Values
> - Denormalized Values
> - Special Values

> **Floating-Point Rounding:**
> - Round to even (IEEE default)
> - Round to zero
> - Round up/down

### Experiment: hello.c, GDB, and objdump

Write, compile, debug, and disassemble a simple C program to inspect generated instructions.

#### 1. Setup

```c
#include <stdio.h>
int main(void) {
    printf("Hello, World!\n");
    return 0;
}
```

#### 2. Compilation

```bash
clang -O2 -std=c23 -Wall -Wextra -fsanitize=undefined -g hello.c -o hello.exe
```

#### 3. GDB Debugging

```bash
gdb ./hello.exe
(gdb) start
(gdb) disassemble main
(gdb) layout regs
(gdb) ni
(gdb) quit
```

- `start` hit temporary breakpoint at `main`
- `main` calls `__main` then `puts`
- Registers viewable with `layout regs`
- Disassembly confirms `printf` compiled to `puts`

#### 4. Objdump

```bash
llvm-objdump -dS hello.exe > hello.S
```

### Reflection: Platform-Dependent Type Sizes

**Q1. Why do `char` / `int` / `long` sizes vary across platforms?**  
Type sizes depend on the platform's word size and its ABI. For example, `long` is 8 bytes on LP64 systems (Linux/macOS), but only 4 bytes on LLP64 systems (Windows).

**Q2. Can compilers choose type sizes freely?**  
No. Compilers must strictly follow the platform's ABI, or the resulting binaries won't work with system libraries or other modules.

**Q3. What is the relation between CPU word size and types like `int`?**  
The word size is the width of data the CPU can process in one operation (e.g., 32-bit or 64-bit). Types like `int` are usually designed to match the word size for performance reasons.

**Q4. Which types are most platform-dependent, and why?**  
`long` and `size_t` vary most across platforms because they reflect memory addressing and are tied to the data model.

**Q5. What are common portability issues related to type sizes?**  
- Assuming `int` is always 4 bytes  
- Using `sizeof(long)` for offset calculations  
- Not using fixed-width types like `int32_t` from `<stdint.h>`  

## Day 2: CSAPP Data Lab

### Environment Setup

```bash
wsl --install -d Ubuntu-22.04

# Inside WSL:
sudo apt update
sudo apt install gcc-multilib g++-multilib libc6-dev-i386
```

### Result

> Score: 62/62 (36/36 Correct + 26/26 Performance)  
> Operators used: 137

### Notes

#### 1. Signed Overflow and Undefined Behavior

```c
// Initial incorrect solution
int isTmax(int x) {
    return !(x + x + 2) & !!(x + 1); // Always returns 0 due to optimization
}
```

This code gives the wrong result because signed integer overflow is undefined behavior (UB). The compiler assumes `x + x + 2 == 0`, which only holds for `x = -1`, making `!!(x + 1)` false. As a result, the expression is optimized to `return 0`. 

```bash
(gdb) disassemble isTmax
# Dump of assembler code for function isTmax:
#    0x0000000000001149 <+0>:     endbr64 
#    0x000000000000114d <+4>:     push   %rbp
#    0x000000000000114e <+5>:     mov    %rsp,%rbp
#    0x0000000000001151 <+8>:     mov    %edi,-0x4(%rbp)
#    0x0000000000001154 <+11>:    mov    $0x0,%eax
#    0x0000000000001159 <+16>:    pop    %rbp
#    0x000000000000115a <+17>:    ret    
# End of assembler dump.
```

Using `volatile` prevents the compiler from applying undefined optimizations:
```c
int isTmaxVolatile(int x) {
    volatile int a = x + x + 2;
    volatile int b = x + 1;
    return !a & !!b;
}
```

```bash
(gdb) disassemble isTmaxVolatile
# Dump of assembler code for function isTmaxVolatile:
#    0x000000000000115b <+0>:     endbr64 
#    0x000000000000115f <+4>:     push   %rbp
#    0x0000000000001160 <+5>:     mov    %rsp,%rbp
#    0x0000000000001163 <+8>:     mov    %edi,-0x14(%rbp)
#    0x0000000000001166 <+11>:    mov    -0x14(%rbp),%eax
#    0x0000000000001169 <+14>:    add    $0x1,%eax
#    0x000000000000116c <+17>:    add    %eax,%eax
#    0x000000000000116e <+19>:    mov    %eax,-0x8(%rbp)
#    0x0000000000001171 <+22>:    mov    -0x14(%rbp),%eax
#    0x0000000000001174 <+25>:    add    $0x1,%eax
```

```c
int main() {
    printf("isTmax(0x7fffffff)         = %d\n", isTmax(0x7fffffff));
    printf("isTmaxVolatile(0x7fffffff) = %d\n", isTmaxVolatile(0x7fffffff));
    return 0;
}
```

```bash
gcc test.c -o test && ./test
# isTmax(0x7fffffff)         = 0
# isTmaxVolatile(0x7fffffff) = 1
```


## Day 3: Assembly Fundamentals & Function Analysis

### Reading Notes: CS:APP 3.1–3.5

#### x86-64 Register Summary
> 
> **General Purpose Registers (64-bit):**
> - `RAX`: Accumulator (return values, arithmetic)
> - `RBX`: Base (callee-saved)
> - `RCX`: Counter (loops, shifts)
> - `RDX`: Data (I/O, argument passing)
> - `RSI`: Source index (memory operations, 2nd argument)
> - `RDI`: Destination index (memory operations, 1st argument)
> - `RBP`: Base/frame pointer (stack frame reference)
> - `RSP`: Stack pointer (top of current stack)
> - `R8`–`R9`: 5th–6th function arguments
> - `R10`–`R11`: Temporaries (caller-saved)
> - `R12`–`R15`: Callee-saved local use

> **Instruction Pointer:**
> - `RIP`: Points to next instruction

> **Flags (Condition Codes):**
> - `EFLAGS` stores results of last arithmetic/logical op:
>   - `ZF` (zero), `SF` (sign), `CF` (carry), `OF` (overflow)

> **SIMD / Vector Registers:**
> - `XMM0`–`XMM15`: 128-bit (SSE, float/int vectors)
> - `YMM0`–`YMM15`: 256-bit (AVX)
> - `ZMM0`–`ZMM31`: 512-bit (AVX-512, if supported)

> **Calling Convention (System V ABI):**
> - Arguments: `RDI`, `RSI`, `RDX`, `RCX`, `R8`, `R9`
> - Return value: `RAX`
> - Caller-saved: `RAX`, `RCX`, `RDX`, `R8–R11`
> - Callee-saved: `RBX`, `RBP`, `R12–R15`

#### Instructions Summary

> **Operand Types**
>
> - Immediate: `$imm` → constant
> - Register: `r_a` → `R[r_a]`
> - Memory:
>   - `imm` → `M[imm]` (absolute)
>   - `(r_a)` → `M[R[r_a]]` (indirect)
>   - `imm(r_b)` → `M[imm + R[r_b]]` (base + displacement)
>   - `(r_b, r_i)` → `M[R[r_b] + R[r_i]]` (indexed)
>   - `imm(r_b, r_i)` → `M[imm + R[r_b] + R[r_i]]`
>   - `(, r_i, s)` → `M[R[r_i] * s]` (scaled index)
>   - `imm(, r_i, s)` → `M[imm + R[r_i] * s]`
>   - `(r_b, r_i, s)` → `M[R[r_b] + R[r_i] * s]`
>   - `imm(r_b, r_i, s)` → `M[imm + R[r_b] + R[r_i] * s]`

> **Data Movement**
>
> - `mov S, D` → `D ← S`
> - `movb/w/l/q` → move byte/word/dword/qword
> - `movabsq I, R` → move absolute immediate into register
>
> **Zero Extension**
>
> - `movz S, R` → `R ← ZeroExtend(S)`
>   - `movzbw`, `movzbl`, `movzwl`, `movzbq`, `movzwq`
>
> **Sign Extension**
>
> - `movs S, R` → `R ← SignExtend(S)`
>   - `movsbw`, `movsbl`, `movswl`, `movsbq`, `movswq`, `movslq`
> - `cltq` → `%rax ← SignExtend(%eax)`
>
> **Stack**
>
> - `pushq S`:
>   - `R[%rsp] ← R[%rsp] - 8`
>   - `M[R[%rsp]] ← S`
> - `popq D`:
>   - `D ← M[R[%rsp]]`
>   - `R[%rsp] ← R[%rsp] + 8`

> **Arithmetic**
>
> - Unary:
>   - `inc D`: `D ← D + 1`
>   - `dec D`: `D ← D - 1`
>   - `neg D`: `D ← -D`
>   - `not D`: `D ← ~D`
>
> - Binary:
>   - `add S, D`: `D ← D + S`
>   - `sub S, D`: `D ← D - S`
>   - `imul S, D`: `D ← D * S`
>   - `xor S, D`: `D ← D ^ S`
>   - `or S, D`: `D ← D | S`
>   - `and S, D`: `D ← D & S`
>
> - Shifts:
>   - `sal` / `shl`: `D ← D << k` (same)
>   - `sar`: `D ← D >>A k` (arithmetic)
>   - `shr`: `D ← D >>L k` (logical)
>
> - `leaq S, D`: `D ← &S` (compute address without dereferencing)

> **Special Arithmetic**
>
> - Multiply:
>   - `imulq S`: `R[%rdx]:R[%rax] ← S * R[%rax]` (signed)
>   - `mulq S`: `R[%rdx]:R[%rax] ← S * R[%rax]` (unsigned)
>
> - Division:
>   - `cqto`: `R[%rdx]:R[%rax] ← SignExtend(R[%rax])`
>   - `idivq S`: Signed divide
>     - `R[%rdx] ← R[%rdx]:R[%rax] % S`
>     - `R[%rax] ← R[%rdx]:R[%rax] / S`
>   - `divq S`: Unsigned divide
>     - Same effect as above

**Notes:**
- Stack grows downward (high → low addresses)
- Registers can store integers or pointers (64-bit)

### Function Dissection Practice

#### Source Code & Complication
```c
int sum(int *a, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) {
        s += a[i];
    }
    return s;
}
```

Compiled with:

```bash
gcc -O0 -g sum.c -o sum_O0
objdump -d -M intel sum_O0 > sum_O0.s

gcc -O2 -g sum.c -o sum_O2
objdump -d -M intel sum_O2 > sum_O2.s
```

#### `sum_O0.s` Walkthrough 

> **Function Prologue**
>
> ```
> push rbp                 # save old base pointer
> mov rbp, rsp             # set up new stack frame
> ```
>
> **Save parameters**
>
> ```
> mov [rbp-0x18], rdi      # int* a
> mov [rbp-0x1c], esi      # int n
> ```
>
> **Initialize locals**
>
> ```
> mov [rbp-0x08], 0        # s = 0
> mov [rbp-0x04], 0        # i = 0
> jmp <loop_cond>
> ```
> 
> **Loop body: `s += a[i]`**
>
> ```
> mov eax, [rbp-0x04]          # eax = i
> cdqe                         # sign-extend to rax
> lea rdx, [rax*4]             # rdx = i * 4
> mov rax, [rbp-0x18]          # rax = a
> add rax, rdx                 # rax = &a[i]
> mov eax, [rax]               # eax = a[i]
> add [rbp-0x08], eax          # s += a[i]
> add [rbp-0x04], 1            # i++
> ```
>
> **Loop condition: `i < n`**
>
> ```
> mov eax, [rbp-0x04]
> cmp eax, [rbp-0x1c]
> jl <loop_body>
> ```
>
> **Return**
>
> ```
> mov eax, [rbp-0x08]          # return s
> pop rbp
> ret
> ```

> **Stack Frame Layout (relative to rbp)**
>
> ```
> [rbp+0x00] → old rbp
> [rbp-0x04] → int i
> [rbp-0x08] → int s
> [rbp-0x1c] → int n
> [rbp-0x18] → int* a
> ```

#### `sum_O2.s` Walkthrough

Compiled with `-O2`, this version eliminates the stack frame and uses only registers for computation.

> **Assembly**
> ```
> test esi, esi               # if (n <= 0)
> jle  <exit_zero>            # return 0
>
> lea eax, [esi - 1]          # eax = n - 1
> lea rdx, [rdi + rax*4 + 4]  # rdx = a + n*4 (end address)
> xor eax, eax                # eax = 0 (sum)
>
> loop:
> add eax, [rdi]              # sum += *a
> add rdi, 4                  # a++
> cmp rdi, rdx                # reached end?
> jne loop
> ret
>
> exit_zero:
> xor eax, eax
> ret
> ```

> **Equivalent C Code**
>
> ```c
> rdx = a + n * 4;   // end pointer
> eax = 0;
> for (; rdi != rdx; rdi += 4)
>     eax += *(int *)rdi;
> return eax;
> ```

- Loop unrolled to pointer comparison and offset addition
- Uses `rdi` as the loop pointer and `rdx` as end address
- No memory access beyond the input array
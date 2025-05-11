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

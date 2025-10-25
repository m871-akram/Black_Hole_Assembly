# Assembly Syntax Fix - AT&T vs Intel/NASM

## What Was Wrong

The original assembly file used **Intel/NASM syntax**, but on Linux with GCC, the GNU Assembler (GAS) expects **AT&T syntax**.

## Key Differences

### Comments
- **Intel/NASM**: `;` for comments
- **AT&T/GAS**: `/* */` or `#` for comments

### Register Names
- **Intel/NASM**: `xmm0`, `rdi`, `rsi`
- **AT&T/GAS**: `%xmm0`, `%rdi`, `%rsi` (with `%` prefix)

### Operand Order
- **Intel/NASM**: `mov destination, source`
- **AT&T/GAS**: `mov source, destination` (**reversed!**)

### Memory Access
- **Intel/NASM**: `[rdi]`, `[rdi + 4]`
- **AT&T/GAS**: `(%rdi)`, `4(%rdi)`

### Immediate Values
- **Intel/NASM**: `mov rax, 5`
- **AT&T/GAS**: `movq $5, %rax` (with `$` prefix)

### Directives
- **Intel/NASM**: `section .text`, `global _funcname`
- **AT&T/GAS**: `.text`, `.globl funcname`

### Function Names
- **macOS**: Requires `_` prefix (`_vector_distance_squared`)
- **Linux**: No prefix (`vector_distance_squared`)

## What Was Fixed

1. ✅ Changed comments from `;` to `/* */`
2. ✅ Added `%` prefix to all registers
3. ✅ Reversed operand order (source, destination)
4. ✅ Changed memory syntax from `[reg]` to `(%reg)`
5. ✅ Changed directives from `section .text` to `.text`
6. ✅ Changed `global` to `.globl`
7. ✅ Removed `_` prefix from function names (Linux compatibility)
8. ✅ Added `.type` and `.size` directives for proper symbol table
9. ✅ Changed label syntax from `.label:` to `.Llabel:` for local labels
10. ✅ Fixed data section from `dd` to `.float`

## Example Transformation

### Before (Intel/NASM):
```asm
_vector_distance_squared:
    subss   xmm3, xmm0      ; dx = x2 - x1
    mulss   xmm3, xmm3      ; dx^2
    movss   xmm0, xmm3      ; return result
    ret
```

### After (AT&T/GAS):
```asm
vector_distance_squared:
    subss   %xmm0, %xmm3    /* dx = x2 - x1 */
    mulss   %xmm3, %xmm3    /* dx^2 */
    movss   %xmm3, %xmm0    /* return result */
    ret
```

## How to Build Now

The assembly file is now compatible with GCC/GAS on Linux:

```bash
cd /home/paperspace/Black_Hole_Assembly
./build_asm_demo.sh
```

Or manually:
```bash
cd cmake-build-debug
cmake ..
make PhysicsASM_Demo
./PhysicsASM_Demo
```

## Platform Compatibility

The fixed assembly code now works on:
- ✅ **Linux** (Ubuntu, Debian, etc.) with GCC
- ✅ **macOS** with Clang (may need minor adjustments for `_` prefix)
- ❌ **Windows** requires different syntax (MASM/NASM)

## Technical Note

AT&T syntax is the default for GNU tools (GCC, GAS) and is widely used in Unix/Linux systems. Intel syntax is more common in Windows environments and with assemblers like NASM and MASM.

The System V AMD64 ABI (calling convention) remains the same regardless of syntax - parameters still go in `%xmm0-7` for floats and `%rdi, %rsi, %rdx` for pointers.


# emex64

## Introduction
emex64 is a 64bit lightweight little endian architecture. It's a mix out of RISC and CISC it is based on no previous architecture.

Outside the SoC, the emulated board additionally integrates support for UART, Audio, and (implementation pending) Graphics.

## Setup and Installation of the emex64 toolchain
Bulding the toolchain and installing it is as simple as the following:

```bash
make && make install
```

This will install emex64's toolchain and VM to `/usr/local`, and will prompt for a superuser password to do so.

emex64vm will additionally require GLFW/GLEW if using the virtual display.

## Using the Virtual Machine (VM)
The VM can be invoked to run firmware with `emex64vm --firmware <image path>`. Test programs and the current testing firmware can be found in `./tests/`.

These examples will be compiled and directly run with `make`. 

## Instruction Set Architecture (ISA)
The instruction coding is variable, it is not a fixed lenght instruction set, which is a CISC concept.

### Register Set (RS)
#### Userspace Accessible Registers
| Register  | Name                                  | Binary       | Description |
|-----------|---------------------------------------|--------------|-------------|
| `pc`      | **P**rogram **C**ounter               | `0b00000`    | Points to the current address at which the CPU currently is, it increments by the lenght of the instruction when the CPU is done executing the instruction at which PC points to at that time. |
| `sp`      | **S**tack **P**ointer                 | `0b00001`    | Points to the current address at which the stack lives, the stack grows downwards on allocation and upwards on deallocation. |
| `fp`      | **F**rame **P**ointer                 | `0b00010`    | Points to the address at which the stack frame of the last function call lives, basically empowering you to branch and link and return back without destroying values stored in registers previously. |
| `cf`      | **C**ontrol **F**lag                  | `0b00011`    | Used by control flow operations like `cmp`, `be` and `bne`. Basically used for if else kind of statements. |
| `fpc`     | **F**loating **Point** **C**ontrol    | `0b00100`    | Controls the behaviour of the implementation pending floating point registers. |
| `r0` - `r25`      | General Purpose Registers             | `0b00101` - `0b11111`   | Use it for what ever. |
| `rr`      | Return Register                       | `0b11111`    | Unaffected by operations like `blw` and `wret`. Intended to be used as a return value register. |

### Opcode Set
(1) Applies mathematical operation either on two or one operand together and stores the result into the source, the source must always be a register and can also be a operand.

(2) Variadic instruction, meaning it can be used to apply the same operation onto many registers at the same time.

#### Core
| Opcode      | Binary       | Description |
|-------------|--------------|-------------|
| `hlt`       | `0b00000000` | Halts the CPU core until the next interrupt occurs from a timer or other device.        |
| `nop`       | `0b00000001` | Does nothing, does a cycle.        |

#### Data
| Opcode      | Binary       | Description |
|-------------|--------------|-------------|
| `mov`       | `0b00000010` | Moves a intermediate or a value of a register into a register. |
| `swp`       | `0b00000011` | Swaps the values of two registers. |
| `swpz`      | `0b00000100` | Swaps the values of two registers, while zeroing out the source. |
| `push`      | `0b00000101` | Pushes a intermediate or a value of a register onto the stack. *(2) |
| `pop`       | `0b00000110` | Pops a intermediate from the stack into a register. *(2) |
| `ldb`       | `0b00000111` | Loads a byte from a memory address into a register. |
| `ldw`       | `0b00001000` | Loads a word from a memory address into a register. |
| `ldd`       | `0b00001001` | Loads a double word from a memory address into a register. |
| `ldq`       | `0b00001010` | Loads a quad word form a memory address into a register. |
| `stb`       | `0b00001011` | Stores a byte from a register into a memory address. |
| `stw`       | `0b00001100` | Stores a word from a register into a memory address. |
| `std`       | `0b00001101` | Stores a double word from a register into a memory address. |
| `stq`       | `0b00001110` | Stores a quad word from a register into a memory address. |

#### ALU
| Opcode      | Binary       | Description  |
|-------------|--------------|--------------|
| `add`       | `0b00001111` | Addition. *(1)   |
| `sub`       | `0b00010000` | Subtraction. *(1)    |
| `mul`       | `0b00010001` | Multiplication. *(1) |
| `div`       | `0b00010010` | Division. *(1)   |
| `idiv`      | `0b00010011` | Signed Division. *(1)    |
| `mod`       | `0b00010100` | Mudolu. *(1) |
| `not`       | `0b00010101` | Applies a bitwise NOT gate onto the operands. *(2) |
| `neg`       | `0b00010110` | Applies arithmetic negation onto the operands. *(2) |
| `and`       | `0b00010111` | AND gate *(1)|
| `or`        | `0b00011000` | OR gate *(1) |
| `xor`       | `0b00011001` | XOR gate *(1)|
| `shr`       | `0b00011010` | Shifts bits to the right. *(1) |
| `shl`       | `0b00011011` | Shifts bits to the left. *(1) |
| `sar`       | `0b00011100` | Shifts bits to the right arithmetically. *(1) |
| `ror`       | `0b00011101` | Rolls bits to the right. *(1) |
| `rol`       | `0b00011110` | Rolls bits to the left. *(1) |
| `pdep`      | `0b00011111` | Extracts non-contiguous bits from a source operand based on a mask pattern. |
| `pext`      | `0b00100000` | Does the reverse of pext. Spreads contiguous bits into non-contiguous positions. |
| `bswapw`    | `0b00100001` | Reverses the byte order of a word. |
| `bswapd`    | `0b00100010` | Reverses the byte order of a double word. |
| `bswapq`    | `0b00100011` | Reverses the byte order of a quad word. |
| `inc`       | `0b00100100` | Increments operands. *(2) |
| `dec`       | `0b00100101` | Decrements operands. *(2) |

#### Control flow
| Opcode      | Binary       | Description      |
|-------------|--------------|------------------|
| `b`         | `0b00100110` | Branches to a address by setting the PC register. |
| `cmp`       | `0b00100111` | Compares two operands and sets the `cf` register. |
| `be`        | `0b00101000` | Branches when the `cf` register says that the compared operands compared using `cmp` were equal. |
| `bne`       | `0b00101001` | Branches when the `cf` register says that the compared operands compared using `cmp` were not equal. |
| `blt`       | `0b00101010` | Branches when the `cf` register says that the first compared operand of the operands compared using `cmp` was less than the second operand. |
| `bgt`       | `0b00101011` | Branches when the `cf` register says that the first compared operand of the operands compared using `cmp` was greater than the second operand. |
| `ble`       | `0b00101100` | Branches when the `cf` register says that the first compared operand of the operands compared using `cmp` was less or equal to the second operand. |
| `bge`       | `0b00101101` | Branches when the `cf` register says that the first compared operand of the operands compared using `cmp` was greater or equal to the second operand. |
| `bz`        | `0b00101110` | Branches when the first operand is zero. |
| `bnz`       | `0b00101111` | Branches when the first operand is not zero. |
| `blw`       | `0b00110000` | Branches and links wastefully to a address by pushing all registers usable in the userspace to the stack. Linkage is done by storing the last stack pointer address to the stack frame in the `fp` register. |
| `wret`      | `0b00110001` | Wastefully returns to the address it branched from when `blw` was used to branch by restoring all previously pushed registers.  |
| `iret`      | `0b00110010` | Returns from a interrupt handler by restoring all registers backedup by the interrupt controller onto the stack located at the kernel stack pointer. |
| `bl`        | `0b00110011` | branches and links by only storing the last `sp` address into the `fp` register. |
| `ret`       | `0b00110100` | Returns from a `bl` branch. |

#### Data (v2)
| Opcode      | Binary       | Description |
|-------------|--------------|-------------|
| `clr`       | `0b00110101` | Clears operands. *(2) |
| `cmov`      | `0b00110110` | Moves a value of a register or intermediate into a control register of the core.  |
| `cmovb`     | `0b00110111` | Moves a value from a control register into a register. |

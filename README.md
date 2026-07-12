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
The VM can be invoked to run firmware with `emex64vm -f <image path>`. Test programs and the current testing firmware can be found in `./tests/`.

These examples will be compiled and directly run with `make`. 

## Instruction Set Architecture (ISA)
The instruction coding is variable, it is not a fixed lenght instruction set, which is a CISC concept. Instructions are coded by the first 8 bit serving as the opcode, followed by operands that are not aligned to a byte boundary. Operands are coded by the first 4 bit serving as the type of operand followed by the operand it self, except it is a offset coding, that means that 2 more operands will follow where the first one is the main operand and the 2nd one is the operand responsible for the offset. A table of the operand types here:

| Type               | Binary       | Description                |
|--------------------|--------------|----------------------------|
| End                | `0b0000`     | Terminates the instruction, parser stops parsing |
| Register           | `0b0001`     | 4 bit register identifier  |
| Immediate (4bit)   | `0b0010`     | 4 bit immediate            |
| Immediate (8bit)   | `0b0011`     | 8 bit immediate            |
| Immediate (16bit)  | `0b0100`     | 16 bit immediate           |
| Immediate (32bit)  | `0b0101`     | 32 bit immediate           |
| Immediate (64bit)  | `0b0110`     | 64 bit immediate           |
| Address (64bit)    | `0b0111`     | 64 bit immediate, with the difference that the decoder aligns to the next byte boundary, this was done with the intention for easy address relocation in the linker, it payed off >:3. |
| Register Extended  | `0b1000`     | 4 bit register identifier to the 2nd register file |
| Register Increment | `0b1001`     | 4 bit register identifier, but decoder increments register after parse |
| Register Decrement | `0b1010`     | 4 bit register identifier, but decoder decrements register after parse |
| Offset Add         | `0b1011`     | Offset Addition |
| Offset Subtract    | `0b1100`     | Offset Subtraction |

In case it is a immediate it stores the immediate into a immediate cache of the core which then gets used as a operand inside of the operation logic. Offsetted operands are always in the immediate cache.

### Register Set (RS)
#### Main Register File
Each of these registers are accessible in userspace aswell as in kernelspace. These registers also support the operand codings like "Register Increment" and "Register Decrement."
| Register  | Name                                  | Binary       | Description |
|-----------|---------------------------------------|--------------|-------------|
| `pc`      | **P**rogram **C**ounter               | `0b0000`     | Points to the current address at which the CPU currently is, it increments by the lenght of the instruction when the CPU is done executing the instruction at which PC points to at that time. |
| `sp`      | **S**tack **P**ointer                 | `0b0001`     | Points to the current address at which the stack lives, the stack grows downwards on allocation and upwards on deallocation. |
| `fp`      | **F**rame **P**ointer                 | `0b0010`     | Points to the address at which the stack frame of the last function call lives, basically empowering you to branch and link and return back without destroying values stored in registers previously. |
| `cf`      | **C**ontrol **F**lag                  | `0b0011`     | Used by control flow operations like `cmp`, `be` and `bne`. Basically used for if else kind of statements. |
| `fpc`     | **F**loating **Point** **C**ontrol    | `0b0100`     | Controls the behaviour of the **implementation pending** floating point registers. |
| `r0` - `r9`      | General Purpose Registers      | `0b0101` - `0b1110`   | Use it for what ever. |
| `rr`      | **R**eturn **R**egister               | `0b1111`    | Unaffected by operations like `blw` and `wret`. Intended to be used as a return value register. |

#### Extended Register File
As only 10 general purpose registers is not much we created a extended register file, but the cavet is that it doesn't support direct decode level manipulation like increment and decrementing in place.
| Register  | Name                                  | Binary       | Description |
|-----------|---------------------------------------|--------------|-------------|
| `er0` - `er15` | Extended General Purpose Registers | `0b0000` - `0b1111`   | Use it for what ever. |

#### Control Register File
| Register  | Name                                                        | Binary       | Description |
|-----------|-------------------------------------------------------------|--------------|-------------|
| `crel`    | **C**ontrol **R**egister **E**leveation **L**evel           | `0b0000`     | Controls the elevation of the core, the higher the value the more priveleged the core is. |
| `crksp`   | **C**ontrol **R**egister **K**ernel **S**tack **P**ointer   | `0b0001`     | Stores the address of the stack base used when the interrupt controller interrupts the core. |
| `crexc`   | **C**ontrol **R**egister **E**xception                      | `0b0010`     | Stores exception information. |
| `crvec`   | **C**ontrol **R**egister **V**ector                         | `0b0011`     | No-Op |
| `crptb`   | **C**ontrol **P**egister **P**age **T**able **B**ase        | `0b0100`     | Is treated by the MMU as the 5th level page table entry. |
| `crfpc`   | **C**ontrol **R**egister **F**loating **P**oint **C**ontrol | `0b0101`     | No-Op |

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
| `mov`       | `0b00000010` | Moves a immediate or a value of a register into a register. |
| `swp`       | `0b00000011` | Swaps the values of two registers. |
| `movz`      | `0b00000100` | Moves the values of two registers, while zeroing out the source. |
| `push`      | `0b00000101` | Pushes a immediate or a value of a register onto the stack. *(2) |
| `pop`       | `0b00000110` | Pops a immediate from the stack into a register. *(2) |
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
| `cmov`      | `0b00110110` | Moves a value of a register or immediate into a control register of the core.  |
| `cmovb`     | `0b00110111` | Moves a value from a control register into a register. |

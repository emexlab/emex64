# emex64

## Introduction
emex64 is a 64bit lightweight architecture. It's a mix out of RISC and CISC it is based on no previous architecture, its only inspired by previous architectures like aarch64.

## Instruction Set
#### Core
| Instruction | Opcode       | Format      |
|-------------|--------------|-------------|
| `hlt`       | `0b00000000` | `op`        |
| `nop`       | `0b00000001` | `op`        |
#### Data
| Instruction | Opcode       | Format      |
|-------------|--------------|-------------|
| `mov`       | `0b00000010` | `op dest, any` |
| `swp`       | `0b00000011` | `op dest, src` |
| `swpz`      | `0b00000100` | `op dest, src` |
| `push`      | `0b00000101` | `op src, ...32`  |
| `pop`       | `0b00000110` | `op dest, ...32` |
| `ldb`       | `0b00000111` | `op dest, any` |
| `ldw`       | `0b00001000` | `op dest, any` |
| `ldd`       | `0b00001001` | `op dest, any` |
| `ldq`       | `0b00001010` | `op dest, any` |
| `stb`       | `0b00001011` | `op any, src` |
| `stw`       | `0b00001100` | `op any, src` |
| `std`       | `0b00001101` | `op any, src` |
| `stq`       | `0b00001110` | `op any, src` |
#### ALU
| Instruction | Opcode       | Format      |
|-------------|--------------|-------------|
| `add`       | `0b00001111` | `op dest, any` or `op dest, any, any` |
| `sub`       | `0b00010000` | `op dest, any` or `op dest, src, any` |
| `mul`       | `0b00010001` | `op dest, any` or `op dest, src, any` |
| `div`       | `0b00010010` | `op dest, any` or `op dest, src, any` |
| `idiv`      | `0b00010011` | `op dest, any` or `op dest, src, any` |
| `mod`       | `0b00010100` | `op dest, any` or `op dest, src, any` |
| `not`       | `0b00010101` | `op dest, ...32` |
| `neg`       | `0b00010110` | `op dest, ...32` |
| `and`       | `0b00010111` | `op dest, any` or `op dest, src, any` |
| `or`        | `0b00011000` | `op dest, any` or `op dest, src, any` |
| `xor`       | `0b00011001` | `op dest, any` or `op dest, src, any` |
| `shr`       | `0b00011010` | `op dest, any` or `op dest, src, any` |
| `shl`       | `0b00011011` | `op dest, any` or `op dest, src, any` |
| `sar`       | `0b00011100` | `op dest, any` or `op dest, src, any` |
| `ror`       | `0b00011101` | `op dest, any` or `op dest, src, any` |
| `rol`       | `0b00011110` | `op dest, any` or `op dest, src, any` |
| `pdep`      | `0b00011111` |             |
| `pext`      | `0b00100000` |             |
| `bswapw`    | `0b00100001` |             |
| `bswapd`    | `0b00100010` |             |
| `bswapq`    | `0b00100011` |             |
### Control flow
| Instruction | Opcode       | Format      |
|-------------|--------------|-------------|
| `b`         | `0b00100100` | `op any` |
| `cmp`       | `0b00100101` | `op any, any` |
| `be`        | `0b00100110` | `op any` |
| `bne`       | `0b00100111` | `op any` |
| `blt`       | `0b00101000` | `op any` |
| `bgt`       | `0b00101001` | `op any` |
| `ble`       | `0b00101010` | `op any` |
| `bge`       | `0b00101011` | `op any` |
| `bz`        | `0b00101100` | `op reg, any` |
| `bnz`       | `0b00101101` | `op reg, any` |
| `bl`        | `0b00101110` | `op any` |
| `ret`       | `0b00101111` | `op` |
| `iret`      | `0b00110000` | `op` |

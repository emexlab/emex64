/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EMEX64VM_DEVICE_SATA_H
#define EMEX64VM_DEVICE_SATA_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/board/pcie/pcie.h>

#define SATA_PCI_DEVICE_ID          0x0001
#define SATA_PCI_CLASS              0x0106E6
#define SATA_BAR0_SIZE              EMEX64_PAGE_SIZE

#define SATA_MAX_PORTS              8
#define SATA_CMD_SLOTS              32
#define SATA_SECTOR_SIZE            512
#define SATA_MAX_SECTORS_PER_CMD    65536

#define SATA_REG_CAP                0x00
#define SATA_REG_GHC                0x08
#define SATA_REG_IS                 0x10
#define SATA_REG_PI                 0x18

#define SATA_GHC_RESET              (1ULL << 0)
#define SATA_GHC_IE                 (1ULL << 1)

/* port registers */
#define SATA_PORT_BASE(p)           (0x100 + (p) * 0x100)
#define SATA_PxSSTS                 0x00
#define SATA_PxSECTORS              0x08
#define SATA_PxSECSZ                0x10
#define SATA_PxCLB                  0x18
#define SATA_PxCMD                  0x20
#define SATA_PxCI                   0x28
#define SATA_PxIS                   0x30
#define SATA_PxIE                   0x38
#define SATA_PxSERR                 0x40

#define SATA_SSTS_PRESENT           (1ULL << 0)
#define SATA_SSTS_READONLY          (1ULL << 1)
#define SATA_DEV_ATA                1

#define SATA_CMD_START              (1ULL << 0)

#define SATA_IS_DONE                (1ULL << 0)
#define SATA_IS_ERROR               (1ULL << 1)

/* command slot */
#define SATA_SLOT_SIZE              64
#define SATA_SLOT_OPCODE            0x00
#define SATA_SLOT_LBA               0x08
#define SATA_SLOT_COUNT             0x10
#define SATA_SLOT_BUFFER            0x18
#define SATA_SLOT_STATUS            0x20
#define SATA_SLOT_BYTES             0x28

#define SATA_OP_READ_DMA_EXT        0x25
#define SATA_OP_WRITE_DMA_EXT       0x35
#define SATA_OP_FLUSH_CACHE_EXT     0xEA
#define SATA_OP_IDENTIFY            0xEC

#define SATA_STATUS_DONE            (1ULL << 0)
#define SATA_STATUS_ERROR           (1ULL << 1)
#define SATA_STATUS_CODE(s)         (((s) >> 8) & 0xFF)

#define SATA_ERR_NONE               0
#define SATA_ERR_BAD_COMMAND        1
#define SATA_ERR_OUT_OF_RANGE       2
#define SATA_ERR_DMA                3
#define SATA_ERR_IO                 4
#define SATA_ERR_READ_ONLY          5
#define SATA_ERR_NO_DEVICE          6

typedef struct {
    int fd;
    Boolean present;
    Boolean readonly;
    UInt64 sectors;
    char model[40];
    char serial[20];
    UInt64 clb, cmd, ci, is, ie, serr;
} emex64_sata_port_t;

typedef struct {
    emex64_pcie_function_t function;
    emex64_sata_port_t ports[SATA_MAX_PORTS];
    UInt64 ghc;
} emex64_sata_t;

typedef struct __E64Machine *E64MachineRef;

emex64_sata_t *emex64_sata_alloc(emex64_pcie_t *pcie, UInt8 dev);
Boolean emex64_sata_attach_disk(emex64_sata_t *sata, UInt8 port, const char *path, Boolean readonly);
void emex64_sata_dealloc(emex64_sata_t *sata);
void emex64_sata_reset(emex64_sata_t *sata);

#endif /* EMEX64VM_DEVICE_SATA_H */

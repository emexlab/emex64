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

#ifndef EMEX64VM_DEVICE_NVME_H
#define EMEX64VM_DEVICE_NVME_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/board/pcie/pcie.h>

#define NVME_PCI_DEVICE_ID          0x0006
#define NVME_PCI_CLASS              0x0108E6
#define NVME_BAR0_SIZE              EMEX64_PAGE_SIZE

#define NVME_MAX_NAMESPACES         4
#define NVME_MAX_QUEUES             17
#define NVME_MAX_QUEUE_ENTRIES      256
#define NVME_LBA_SHIFT              9
#define NVME_LBA_SIZE               (1 << NVME_LBA_SHIFT)
#define NVME_MDTS                   8
#define NVME_MAX_AERS               4

/* registers */
#define NVME_REG_CAP                0x00
#define NVME_REG_VS                 0x08
#define NVME_REG_INTMS              0x10
#define NVME_REG_INTMC              0x18
#define NVME_REG_CC                 0x20
#define NVME_REG_CSTS               0x28
#define NVME_REG_AQA                0x30
#define NVME_REG_ASQ                0x38
#define NVME_REG_ACQ                0x40
#define NVME_REG_SQ_TAIL(q)         (0x1000 + (q) * 0x10)
#define NVME_REG_CQ_HEAD(q)         (0x1008 + (q) * 0x10)

#define NVME_VERSION                0x00010400ULL

#define NVME_CC_EN                  (1ULL << 0)
#define NVME_CC_MPS(cc)             (((cc) >> 7) & 0xF)
#define NVME_CC_SHN(cc)             (((cc) >> 14) & 0x3)
#define NVME_CC_IOSQES(cc)          (((cc) >> 16) & 0xF)
#define NVME_CC_IOCQES(cc)          (((cc) >> 20) & 0xF)

#define NVME_CSTS_RDY               (1ULL << 0)
#define NVME_CSTS_CFS               (1ULL << 1)
#define NVME_CSTS_SHST_DONE         (2ULL << 2)

#define NVME_SQE_SIZE               64
#define NVME_CQE_SIZE               16

#define NVME_ADM_DELETE_SQ          0x00
#define NVME_ADM_CREATE_SQ          0x01
#define NVME_ADM_GET_LOG_PAGE       0x02
#define NVME_ADM_DELETE_CQ          0x04
#define NVME_ADM_CREATE_CQ          0x05
#define NVME_ADM_IDENTIFY           0x06
#define NVME_ADM_ABORT              0x08
#define NVME_ADM_SET_FEATURES       0x09
#define NVME_ADM_GET_FEATURES       0x0A
#define NVME_ADM_ASYNC_EVENT        0x0C

#define NVME_CMD_FLUSH              0x00
#define NVME_CMD_WRITE              0x01
#define NVME_CMD_READ               0x02
#define NVME_CMD_WRITE_ZEROES       0x08
#define NVME_CMD_DSM                0x09

#define NVME_SC_SUCCESS             0x000
#define NVME_SC_INVALID_OPCODE      0x001
#define NVME_SC_INVALID_FIELD       0x002
#define NVME_SC_DATA_XFER_ERROR     0x004
#define NVME_SC_INTERNAL            0x006
#define NVME_SC_INVALID_NS          0x00B
#define NVME_SC_LBA_RANGE           0x080
#define NVME_SC_INVALID_CQ          0x100
#define NVME_SC_INVALID_QID         0x101
#define NVME_SC_INVALID_QSIZE       0x102
#define NVME_SC_INVALID_LOG_PAGE    0x109
#define NVME_SC_INVALID_QDEL        0x10C
#define NVME_SC_NS_WRITE_PROTECTED  0x120
#define NVME_SC_DNR                 0x4000

typedef struct {
    int fd;
    Boolean present;
    Boolean readonly;
    UInt64 blocks;
    char serial[20];
    UInt8 nguid[16];
} emex64_nvme_ns_t;

typedef struct {
    Boolean valid;
    UInt64 base;
    UInt32 size;
    UInt32 head, tail;
    UInt16 cqid;
} emex64_nvme_sq_t;

typedef struct {
    Boolean valid;
    Boolean irq;
    Boolean phase;
    UInt64 base;
    UInt32 size;
    UInt32 head, tail;
} emex64_nvme_cq_t;

typedef struct {
    emex64_pcie_function_t function;
    emex64_nvme_ns_t ns[NVME_MAX_NAMESPACES];
    UInt8 nsCount;
    UInt64 cc, csts, aqa, asq, acq, intms;
    UInt64 pageSize;
    emex64_nvme_sq_t sq[NVME_MAX_QUEUES];
    emex64_nvme_cq_t cq[NVME_MAX_QUEUES];
    UInt16 aerCid[NVME_MAX_AERS];
    UInt8 aerCount;
    UInt32 features[0x10];
    UInt64 unitsRead, unitsWritten, readCommands, writeCommands;
    char serial[20];
} emex64_nvme_t;

emex64_nvme_t *emex64_nvme_alloc(emex64_pcie_t *pcie, UInt8 dev);
Boolean emex64_nvme_attach_nand(emex64_nvme_t *nvme, const char *path, Boolean readonly);
void emex64_nvme_dealloc(emex64_nvme_t *nvme);
void emex64_nvme_reset(emex64_nvme_t *nvme);

#endif /* EMEX64VM_DEVICE_NVME_H */

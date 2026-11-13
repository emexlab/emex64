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

#ifndef EMEX64VM_DEVICE_SB16_H
#define EMEX64VM_DEVICE_SB16_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64MachineOptions.h>
#include <EmexToolchain/VM/device/board/pcie/pcie.h>

#define SB16_PCI_DEVICE_ID          0x0005
#define SB16_PCI_CLASS              0x0401E6
#define SB16_BAR0_SIZE              EMEX64_PAGE_SIZE

#define SB16_PORT(n)                ((n) * 8)
#define SB16_REG_FM_ADDR            SB16_PORT(0x0)
#define SB16_REG_FM_DATA            SB16_PORT(0x1)
#define SB16_REG_FM_ADDR2           SB16_PORT(0x2)
#define SB16_REG_FM_DATA2           SB16_PORT(0x3)
#define SB16_REG_ADLIB_ADDR         SB16_PORT(0x8)
#define SB16_REG_ADLIB_DATA         SB16_PORT(0x9)
#define SB16_REG_MIXER_ADDR         SB16_PORT(0x4)
#define SB16_REG_MIXER_DATA         SB16_PORT(0x5)
#define SB16_REG_RESET              SB16_PORT(0x6)
#define SB16_REG_READ_DATA          SB16_PORT(0xA)
#define SB16_REG_WRITE              SB16_PORT(0xC)
#define SB16_REG_READ_STATUS        SB16_PORT(0xE)
#define SB16_REG_ACK16              SB16_PORT(0xF)

/* instead of the DMA controller */
#define SB16_REG_DMA_ADDR           0x100
#define SB16_REG_DMA_SIZE           0x108
#define SB16_REG_DMA_POS            0x110
#define SB16_REG_FM_QUEUE           0x120
#define SB16_REG_FM_QUEUED          0x128
#define SB16_FM_QUEUE_SIZE          8192

#define SB16_DSP_VERSION_MAJOR      4
#define SB16_DSP_VERSION_MINOR      5

#define SB16_MIXER_RESET            0x00
#define SB16_MIXER_MASTER_LEFT      0x30
#define SB16_MIXER_MASTER_RIGHT     0x31
#define SB16_MIXER_VOICE_LEFT       0x32
#define SB16_MIXER_VOICE_RIGHT      0x33
#define SB16_MIXER_FM_LEFT          0x34
#define SB16_MIXER_FM_RIGHT         0x35
#define SB16_MIXER_IRQ_SELECT       0x80
#define SB16_MIXER_DMA_SELECT       0x81
#define SB16_MIXER_IRQ_STATUS       0x82

typedef struct emex64_sb16 emex64_sb16_t;

emex64_sb16_t *emex64_sb16_alloc(emex64_pcie_t *pcie, UInt8 dev, const E64MachineOptions *options);
void emex64_sb16_dealloc(emex64_sb16_t *sb);
void emex64_sb16_reset(emex64_sb16_t *sb);

#endif /* EMEX64VM_DEVICE_SB16_H */

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

#ifndef EMEX64VM_DEVICE_GPU_H
#define EMEX64VM_DEVICE_GPU_H

#include <pthread.h>
#include <time.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/board/pcie/pcie.h>

#define GPU_PCI_DEVICE_ID           0x0003
#define GPU_PCI_CLASS               0x0380E6
#define GPU_BAR0_SIZE               EMEX64_PAGE_SIZE

/* global registers */
#define GPU_REG_CAP                 0x000
#define GPU_REG_IS                  0x008
#define GPU_REG_IE                  0x010
#define GPU_REG_FRAME               0x018

#define GPU_IRQ_VBLANK              (1ULL << 0)
#define GPU_IRQ_HOTPLUG             (1ULL << 1)

/* DisplayPort connector 0 */
#define GPU_DP_STATUS               0x100
#define GPU_DP_LINK                 0x108
#define GPU_DP_AUX_ADDR             0x110
#define GPU_DP_AUX_DATA             0x118

#define GPU_DP_HPD                  (1ULL << 0)
#define GPU_DP_LINK_UP              (1ULL << 1)
#define GPU_AUX_EDID                (1ULL << 31)

/* scanout */
#define GPU_SCAN_CTRL               0x200
#define GPU_SCAN_BASE               0x208
#define GPU_SCAN_WIDTH              0x210
#define GPU_SCAN_HEIGHT             0x218
#define GPU_SCAN_STRIDE             0x220
#define GPU_SCAN_FORMAT             0x228
#define GPU_SCAN_STATUS             0x230

#define GPU_SCAN_ENABLE             (1ULL << 0)
#define GPU_SCAN_ACTIVE             (1ULL << 0)
#define GPU_SCAN_INVALID            (1ULL << 1)

#define GPU_FORMAT_I8               1
#define GPU_FORMAT_XRGB8888         2

#define GPU_MAX_DIM                 4096
#define GPU_PALETTE                 0x1000
#define GPU_PALETTE_ENTRIES         256

#define GPU_EDID_SIZE               128
#define GPU_DPCD_SIZE               16

typedef struct emex64_display emex64_display_t;

typedef struct {
    emex64_pcie_function_t function;
    emex64_display_t *monitor;
    UInt8 *ram;
    UInt64 ram_size;
    struct emex64_snoop *snoop;
    UInt64 watch_first, watch_end;
    UInt64 last_lo, last_hi;

    pthread_mutex_t lock;
    struct timespec reset_time;

    UInt64 is, ie;
    Boolean link_up;
    UInt64 aux_addr;

    UInt64 scan_ctrl, scan_base, scan_width, scan_height, scan_stride, scan_format;
    Boolean scan_valid;

    UInt8 palette[GPU_PALETTE_ENTRIES * 3];
    UInt32 palette_gen;

    UInt8 edid[GPU_EDID_SIZE];
    UInt8 dpcd[GPU_DPCD_SIZE];
} emex64_gpu_t;

typedef struct {
    Boolean active;
    const UInt8 *pixels;
    UInt32 width, height, stride, format;
    UInt32 dirty_top, dirty_bottom;
    UInt32 palette_gen;
    UInt8 palette[GPU_PALETTE_ENTRIES * 3];
} emex64_gpu_frame_t;

emex64_gpu_t *emex64_gpu_alloc(emex64_pcie_t *pcie, UInt8 dev, emex64_display_t *monitor);
void emex64_gpu_dealloc(emex64_gpu_t *gpu);
void emex64_gpu_reset(emex64_gpu_t *gpu);
void emex64_gpu_get_frame(emex64_gpu_t *gpu, emex64_gpu_frame_t *frame);
void emex64_gpu_vblank(emex64_gpu_t *gpu);

#endif /* EMEX64VM_DEVICE_GPU_H */

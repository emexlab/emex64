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

#include <stdlib.h>
#include <string.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/__E64Cache.h>
#include <EmexToolchain/VM/device/board/display.h>
#include <EmexToolchain/VM/device/board/pcie/gpu.h>

/* sowwy Emily, couldn't wait on vimmio =< */

static void gpu_build_edid(UInt8 e[GPU_EDID_SIZE],
                           UInt16 width,
                           UInt16 height)
{
    static const UInt8 header[8] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
    memset(e, 0, GPU_EDID_SIZE);
    memcpy(e, header, 8);

    UInt16 mfg = (UInt16)((('E' - '@') << 10) | (('M' - '@') << 5) | ('X' - '@'));
    e[8] = (UInt8)(mfg >> 8);
    e[9] = (UInt8)mfg;
    e[10] = 0x64;
    e[11] = 0x00;
    e[12] = 1;
    e[16] = 45;
    e[17] = 2026 - 1990;
    e[18] = 1;
    e[19] = 4;

    e[20] = 0xA5;
    UInt32 wmm = (UInt32)width * 254 / 960;
    UInt32 hmm = (UInt32)height * 254 / 960;
    e[21] = (UInt8)((wmm + 5) / 10);
    e[22] = (UInt8)((hmm + 5) / 10);
    e[23] = 120;
    e[24] = 0x06;

    static const UInt8 chroma[10] = { 0xEE, 0x91, 0xA3, 0x54, 0x4C, 0x99, 0x26, 0x0F, 0x50, 0x54 };
    memcpy(e + 25, chroma, 10);

    for(int i = 38; i < 54; i++)
    {
        e[i] = 0x01;
    }

    UInt32 hblank = 160, hfront = 48, hsync = 32;
    UInt32 vblank = 30, vfront = 3, vsync = 6;
    UInt32 clock = ((UInt32)width + hblank) * ((UInt32)height + vblank) * 60 / 10000;
    UInt8 *d = e + 54;
    d[0] = (UInt8)clock;
    d[1] = (UInt8)(clock >> 8);
    d[2] = (UInt8)width;
    d[3] = (UInt8)hblank;
    d[4] = (UInt8)(((width >> 8) & 0xF) << 4 | ((hblank >> 8) & 0xF));
    d[5] = (UInt8)height;
    d[6] = (UInt8)vblank;
    d[7] = (UInt8)(((height >> 8) & 0xF) << 4 | ((vblank >> 8) & 0xF));
    d[8] = (UInt8)hfront;
    d[9] = (UInt8)hsync;
    d[10] = (UInt8)((vfront & 0xF) << 4 | (vsync & 0xF));
    d[11] = (UInt8)(((hfront >> 8) & 3) << 6 | ((hsync >> 8) & 3) << 4 | ((vfront >> 4) & 3) << 2 | ((vsync >> 4) & 3));
    d[12] = (UInt8)wmm;
    d[13] = (UInt8)hmm;
    d[14] = (UInt8)(((wmm >> 8) & 0xF) << 4 | ((hmm >> 8) & 0xF));
    d[17] = 0x1E;

    static const char name[13] = "emex64 DP\n   ";
    d = e + 72;
    d[3] = 0xFC;
    memcpy(d + 5, name, 13);

    d = e + 90;
    d[3] = 0xFD;
    d[5] = 50;
    d[6] = 75;
    d[7] = 30;
    d[8] = 160;
    d[9] = 30;
    d[10] = 0x01;
    d[11] = 0x0A;
    memset(d + 12, 0x20, 6);

    e[108 + 3] = 0x10;

    e[126] = 0;
    UInt8 sum = 0;
    for(int i = 0; i < 127; i++)
    {
        sum += e[i];
    }
    e[127] = (UInt8)(0x100 - sum);
}

static void gpu_build_dpcd(UInt8 d[GPU_DPCD_SIZE])
{
    memset(d, 0, GPU_DPCD_SIZE);
    d[0x00] = 0x14;
    d[0x01] = 0x1E;
    d[0x02] = 0x84;
    d[0x03] = 0x01;
    d[0x05] = 0x00;
    d[0x07] = 0x00;
}

static Boolean gpu_hpd(emex64_gpu_t *gpu)
{
    return gpu->monitor != NULL && gpu->monitor->enabled;
}

static void gpu_validate_scanout(emex64_gpu_t *gpu)
{
    gpu->scan_valid = false;
    if(!(gpu->scan_ctrl & GPU_SCAN_ENABLE))
    {
        return;
    }

    UInt64 bpp = gpu->scan_format == GPU_FORMAT_I8 ? 1 : gpu->scan_format == GPU_FORMAT_XRGB8888 ? 4 : 0;
    if(bpp == 0 ||
       gpu->scan_width == 0 || gpu->scan_width > GPU_MAX_DIM ||
       gpu->scan_height == 0 || gpu->scan_height > GPU_MAX_DIM ||
       gpu->scan_stride < gpu->scan_width * bpp || gpu->scan_stride > GPU_MAX_DIM * 4)
    {
        return;
    }

    UInt64 size = gpu->scan_stride * (gpu->scan_height - 1) + gpu->scan_width * bpp;
    if(gpu->scan_base >= gpu->ram_size || size > gpu->ram_size - gpu->scan_base)
    {
        return;
    }
    gpu->scan_valid = true;
}

static void gpu_reset(emex64_gpu_t *gpu)
{
    pthread_mutex_lock(&gpu->lock);
    gpu->is = 0;
    gpu->ie = 0;
    gpu->link_up = false;
    gpu->aux_addr = 0;
    gpu->scan_ctrl = 0;
    gpu->scan_base = 0;
    gpu->scan_width = gpu->scan_height = gpu->scan_stride = 0;
    gpu->scan_format = GPU_FORMAT_I8;
    gpu->scan_valid = false;
    for(int i = 0; i < GPU_PALETTE_ENTRIES; i++)
    {
        gpu->palette[i * 3 + 0] = (UInt8)i;
        gpu->palette[i * 3 + 1] = (UInt8)i;
        gpu->palette[i * 3 + 2] = (UInt8)i;
    }
    gpu->palette_gen++;
    clock_gettime(CLOCK_MONOTONIC, &gpu->reset_time);
    pthread_mutex_unlock(&gpu->lock);
}

static void gpu_update_irq(emex64_gpu_t *gpu)
{
    if(gpu->is & gpu->ie)
    {
        emex64_pcie_raise_irq(&gpu->function);
    }
}

static UInt8 gpu_aux_byte(emex64_gpu_t *gpu,
                          UInt64 addr)
{
    if(!gpu_hpd(gpu))
    {
        return 0;
    }
    if(addr & GPU_AUX_EDID)
    {
        addr &= ~GPU_AUX_EDID;
        return addr < GPU_EDID_SIZE ? gpu->edid[addr] : 0;
    }
    return addr < GPU_DPCD_SIZE ? gpu->dpcd[addr] : 0;
}

static UInt64 gpu_bar_read(emex64_pcie_function_t *function,
                           UInt8 bar,
                           UInt64 offset)
{
    emex64_gpu_t *gpu = (emex64_gpu_t *)function->context;
    UInt64 value = 0;
    if(bar != 0)
    {
        return 0;
    }

    pthread_mutex_lock(&gpu->lock);
    if(offset >= GPU_PALETTE && offset < GPU_PALETTE + GPU_PALETTE_ENTRIES * 8)
    {
        UInt64 i = (offset - GPU_PALETTE) / 8;
        value = (UInt64)gpu->palette[i * 3] << 16 | (UInt64)gpu->palette[i * 3 + 1] << 8 | gpu->palette[i * 3 + 2];
        pthread_mutex_unlock(&gpu->lock);
        return value;
    }

    switch(offset)
    {
        case GPU_REG_CAP:
            value = 1 | (UInt64)0x03 << 8 | (UInt64)(GPU_MAX_DIM / 16) << 16 | (UInt64)1 << 32;
            break;
        case GPU_REG_IS:
            value = gpu->is;
            break;
        case GPU_REG_IE:
            value = gpu->ie;
            break;
        case GPU_REG_FRAME:
        {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            Float64 t = (Float64)(now.tv_sec - gpu->reset_time.tv_sec) + (Float64)(now.tv_nsec - gpu->reset_time.tv_nsec) / 1e9;
            value = (UInt64)(t * 60.0);
            break;
        }
        case GPU_DP_STATUS:
            value = (gpu_hpd(gpu) ? GPU_DP_HPD : 0) | (gpu->link_up && gpu_hpd(gpu) ? GPU_DP_LINK_UP : 0);
            break;
        case GPU_DP_LINK:
            value = gpu->link_up && gpu_hpd(gpu) ? (4 | (UInt64)0x1E << 8) : 0;
            break;
        case GPU_DP_AUX_ADDR:
            value = gpu->aux_addr;
            break;
        case GPU_DP_AUX_DATA:
            for(int k = 0; k < 8; k++)
            {
                value |= (UInt64)gpu_aux_byte(gpu, gpu->aux_addr + k) << (8 * k);
            }
            gpu->aux_addr += 8;
            break;
        case GPU_SCAN_CTRL:
            value = gpu->scan_ctrl;
            break;
        case GPU_SCAN_BASE:
            value = gpu->scan_base;
            break;
        case GPU_SCAN_WIDTH:
            value = gpu->scan_width;
            break;
        case GPU_SCAN_HEIGHT:
            value = gpu->scan_height;
            break;
        case GPU_SCAN_STRIDE:
            value = gpu->scan_stride;
            break;
        case GPU_SCAN_FORMAT:
            value = gpu->scan_format;
            break;
        case GPU_SCAN_STATUS:
            if(gpu->scan_ctrl & GPU_SCAN_ENABLE)
            {
                value = gpu->scan_valid ? GPU_SCAN_ACTIVE : GPU_SCAN_INVALID;
            }
            break;
        default:
            break;
    }
    pthread_mutex_unlock(&gpu->lock);
    return value;
}

static void gpu_bar_write(emex64_pcie_function_t *function,
                          UInt8 bar,
                          UInt64 offset,
                          UInt64 value)
{
    emex64_gpu_t *gpu = (emex64_gpu_t *)function->context;
    if(bar != 0)
    {
        return;
    }

    pthread_mutex_lock(&gpu->lock);
    if(offset >= GPU_PALETTE && offset < GPU_PALETTE + GPU_PALETTE_ENTRIES * 8)
    {
        UInt64 i = (offset - GPU_PALETTE) / 8;
        gpu->palette[i * 3 + 0] = (UInt8)(value >> 16);
        gpu->palette[i * 3 + 1] = (UInt8)(value >> 8);
        gpu->palette[i * 3 + 2] = (UInt8)value;
        gpu->palette_gen++;
        pthread_mutex_unlock(&gpu->lock);
        return;
    }

    switch(offset)
    {
        case GPU_REG_IS:
            gpu->is &= ~value;
            break;
        case GPU_REG_IE:
            gpu->ie = value & (GPU_IRQ_VBLANK | GPU_IRQ_HOTPLUG);
            gpu_update_irq(gpu);
            break;
        case GPU_DP_LINK:
            gpu->link_up = (value & 1) && gpu_hpd(gpu);
            break;
        case GPU_DP_AUX_ADDR:
            gpu->aux_addr = value;
            break;
        case GPU_SCAN_CTRL:
            gpu->scan_ctrl = value & GPU_SCAN_ENABLE;
            gpu_validate_scanout(gpu);
            break;
        case GPU_SCAN_BASE:
            gpu->scan_base = value;
            gpu_validate_scanout(gpu);
            break;
        case GPU_SCAN_WIDTH:
            gpu->scan_width = value;
            gpu_validate_scanout(gpu);
            break;
        case GPU_SCAN_HEIGHT:
            gpu->scan_height = value;
            gpu_validate_scanout(gpu);
            break;
        case GPU_SCAN_STRIDE:
            gpu->scan_stride = value;
            gpu_validate_scanout(gpu);
            break;
        case GPU_SCAN_FORMAT:
            gpu->scan_format = value;
            gpu_validate_scanout(gpu);
            break;
        default:
            break;
    }
    pthread_mutex_unlock(&gpu->lock);
}

static const emex64_pcie_ops_t gpu_ops = {
    .bar_read = gpu_bar_read,
    .bar_write = gpu_bar_write,
};

static void gpu_watch(emex64_gpu_t *gpu,
                      UInt64 first,
                      UInt64 end,
                      Boolean watch)
{
    for(UInt64 c = first; c < end; c++)
    {
        if(watch)
        {
            emex64_snoop_mark(gpu->snoop, c << EMEX64_CHUNK_SHIFT, EMEX64_CHUNK_SCANOUT);
        }
        else
        {
            __atomic_fetch_and(&gpu->snoop->flags[c], (UInt8)~EMEX64_CHUNK_SCANOUT, __ATOMIC_ACQ_REL);
        }
    }
}

static void gpu_dirty_rows(emex64_gpu_t *gpu,
                           emex64_gpu_frame_t *frame)
{
    frame->dirty_top = 0;
    frame->dirty_bottom = 0;
    if(gpu->snoop == NULL)
    {
        frame->dirty_bottom = frame->height;
        return;
    }

    UInt64 first = 0, end = 0;
    if(frame->active && frame->stride != 0 && frame->height != 0)
    {
        UInt64 len = (UInt64)frame->stride * frame->height;
        first = gpu->scan_base >> EMEX64_CHUNK_SHIFT;
        end = ((gpu->scan_base + len - 1) >> EMEX64_CHUNK_SHIFT) + 1;
    }

    UInt64 lo = __atomic_exchange_n(&gpu->snoop->scan_lo, UINT64_MAX, __ATOMIC_ACQ_REL);
    UInt64 hi = __atomic_exchange_n(&gpu->snoop->scan_hi, 0, __ATOMIC_ACQ_REL);

    if(first != gpu->watch_first || end != gpu->watch_end)
    {
        gpu_watch(gpu, gpu->watch_first, gpu->watch_end, false);
        gpu_watch(gpu, first, end, true);
        gpu->watch_first = first;
        gpu->watch_end = end;
        gpu->last_lo = first;
        gpu->last_hi = end;
        frame->dirty_bottom = end > first ? frame->height : 0;
        return;
    }

    if(lo < first)
    {
        lo = first;
    }
    if(hi > end)
    {
        hi = end;
    }
    if(lo < hi)
    {
        gpu_watch(gpu, lo, hi, true);
    }

    /* this frame's range and the last one's again */
    UInt64 now_lo = lo, now_hi = hi;
    if(gpu->last_lo < gpu->last_hi)
    {
        if(lo >= hi)
        {
            lo = gpu->last_lo;
            hi = gpu->last_hi;
        }
        else
        {
            lo = lo < gpu->last_lo ? lo : gpu->last_lo;
            hi = hi > gpu->last_hi ? hi : gpu->last_hi;
        }
    }
    gpu->last_lo = now_lo;
    gpu->last_hi = now_hi;
    if(lo >= hi)
    {
        return;
    }

    UInt64 top = ((lo << EMEX64_CHUNK_SHIFT) > gpu->scan_base) ? ((lo << EMEX64_CHUNK_SHIFT) - gpu->scan_base) / frame->stride : 0;
    UInt64 bottom = ((hi << EMEX64_CHUNK_SHIFT) - gpu->scan_base + frame->stride - 1) / frame->stride;
    frame->dirty_top = (UInt32)(top < frame->height ? top : frame->height);
    frame->dirty_bottom = (UInt32)(bottom < frame->height ? bottom : frame->height);
}

void emex64_gpu_get_frame(emex64_gpu_t *gpu,
                          emex64_gpu_frame_t *frame)
{
    pthread_mutex_lock(&gpu->lock);

    frame->active = gpu->scan_valid && gpu->link_up && gpu_hpd(gpu) && (gpu->function.control & PCIE_CONTROL_MASTER);
    frame->pixels = frame->active ? gpu->ram + gpu->scan_base : NULL;
    frame->width = (UInt32)gpu->scan_width;
    frame->height = (UInt32)gpu->scan_height;
    frame->stride = (UInt32)gpu->scan_stride;
    frame->format = (UInt32)gpu->scan_format;
    if(frame->palette_gen != gpu->palette_gen)
    {
        memcpy(frame->palette, gpu->palette, sizeof(frame->palette));
        frame->palette_gen = gpu->palette_gen;
    }

    gpu_dirty_rows(gpu, frame);

    pthread_mutex_unlock(&gpu->lock);
}

void emex64_gpu_vblank(emex64_gpu_t *gpu)
{
    pthread_mutex_lock(&gpu->lock);
    gpu->is |= GPU_IRQ_VBLANK;
    gpu_update_irq(gpu);
    pthread_mutex_unlock(&gpu->lock);
}

emex64_gpu_t *emex64_gpu_alloc(emex64_pcie_t *pcie,
                               UInt8 dev,
                               emex64_display_t *monitor)
{
    emex64_gpu_t *gpu = calloc(1, sizeof(emex64_gpu_t));
    if(gpu == NULL)
    {
        return NULL;
    }

    pthread_mutex_init(&gpu->lock, NULL);
    gpu->monitor = monitor;
    gpu->ram = E64MemoryMapPhysical(pcie->machine->memory, 0, 1, false);
    gpu->ram_size = E64MemoryGetSize(pcie->machine->memory);
    gpu->snoop = E64MemoryGetSnoop(pcie->machine->memory);

    gpu->function.vendor = PCIE_VENDOR_EMEXLABS;
    gpu->function.device = GPU_PCI_DEVICE_ID;
    gpu->function.class_code = GPU_PCI_CLASS;
    gpu->function.revision = 1;
    gpu->function.bar_size[0] = GPU_BAR0_SIZE;
    gpu->function.ops = &gpu_ops;
    gpu->function.context = gpu;

    if(monitor != NULL)
    {
        gpu_build_edid(gpu->edid, monitor->width, monitor->height);
    }
    gpu_build_dpcd(gpu->dpcd);

    if(gpu->ram == NULL || !emex64_pcie_attach(pcie, &gpu->function, dev, 0))
    {
        pthread_mutex_destroy(&gpu->lock);
        free(gpu);
        return NULL;
    }

    gpu_reset(gpu);
    if(monitor != NULL)
    {
        monitor->gpu = gpu;
    }
    return gpu;
}

void emex64_gpu_reset(emex64_gpu_t *gpu)
{
    if(gpu != NULL)
    {
        gpu_reset(gpu);
    }
}

void emex64_gpu_dealloc(emex64_gpu_t *gpu)
{
    if(gpu == NULL)
    {
        return;
    }
    if(gpu->monitor != NULL)
    {
        gpu->monitor->gpu = NULL;
    }
    pthread_mutex_destroy(&gpu->lock);
    free(gpu);
}

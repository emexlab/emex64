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
#include <pthread.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/device/board/sound.h>
#include <EmexToolchain/VM/device/board/opl3/opl3.h>
#include <EmexToolchain/VM/device/board/pcie/sb16.h>

#define SB16_OUT_BUFFER             64
#define SB16_IRQ_8                  (1 << 0)
#define SB16_IRQ_16                 (1 << 1)

struct emex64_sb16 {
    emex64_pcie_function_t function;

    pthread_mutex_t lock;
    emex64_sound_out_t *out;
    Boolean out_tried;
    const char *wav_path;

    UInt8 reset_reg;
    UInt8 cmd;
    UInt8 args[3];
    UInt8 arg_need;
    UInt8 arg_have;
    UInt8 reply[SB16_OUT_BUFFER];
    UInt8 reply_head;
    UInt8 reply_count;
    UInt8 test_reg;
    Boolean speaker;
    UInt32 rate;
    UInt32 block8;

    Boolean playing;
    Boolean paused;
    Boolean bits16;
    Boolean stereo;
    Boolean is_signed;
    Boolean autoinit;
    UInt32 block_bytes;
    UInt32 block_left;
    UInt64 dma_addr;
    UInt64 dma_size;
    UInt64 dma_pos;
    UInt8 irq_status;

    UInt64 phase;
    SInt32 cur[2];
    SInt32 next[2];

    UInt8 mixer_addr;
    UInt8 mixer[256];

    opl3_chip fm;
    Boolean fm_used;
    UInt16 fm_addr[2];
    UInt8 fm_timer_ctrl;
    UInt8 fm_status;
    UInt64 fm_now;
    UInt64 fm_due;
    struct {
        UInt64 at;
        UInt16 reg;
        UInt8 value;
    } fm_queue[SB16_FM_QUEUE_SIZE];
    UInt32 fm_head;
    UInt32 fm_count;
};

static void sb16_fm_reset(emex64_sb16_t *sb)
{
    OPL3_Reset(&sb->fm, EMEX64_SOUND_RATE);
    sb->fm_used = false;
    sb->fm_addr[0] = sb->fm_addr[1] = 0;
    sb->fm_timer_ctrl = 0;
    sb->fm_status = 0;
    sb->fm_now = 0;
    sb->fm_due = 0;
    sb->fm_head = 0;
    sb->fm_count = 0;
}

static void sb16_fm_write(emex64_sb16_t *sb,
                          UInt16 reg,
                          UInt8 value)
{
    sb->fm_used = true;
    if(reg == 0x04)
    {
        if(value & 0x80)
        {
            sb->fm_status = 0;
        }
        else
        {
            sb->fm_timer_ctrl = value;
            if((value & 0x01) && !(value & 0x40))
            {
                sb->fm_status |= 0xC0;
            }
            if((value & 0x02) && !(value & 0x20))
            {
                sb->fm_status |= 0xA0;
            }
        }
        return;
    }
    OPL3_WriteReg(&sb->fm, reg, value);
}

static void sb16_fm_enqueue(emex64_sb16_t *sb,
                            UInt64 entry)
{
    UInt64 now = sb->fm_now * 1000000ULL;
    if(sb->fm_count == 0 && sb->fm_due < now)
    {
        sb->fm_due = now;
    }
    sb->fm_due += (entry >> 32) * EMEX64_SOUND_RATE;
    sb->fm_used = true;

    if(sb->fm_count == SB16_FM_QUEUE_SIZE)
    {
        sb16_fm_write(sb, sb->fm_queue[sb->fm_head].reg, sb->fm_queue[sb->fm_head].value);
        sb->fm_head = (sb->fm_head + 1) % SB16_FM_QUEUE_SIZE;
        sb->fm_count--;
    }
    UInt32 slot = (sb->fm_head + sb->fm_count) % SB16_FM_QUEUE_SIZE;
    sb->fm_queue[slot].at = sb->fm_due / 1000000ULL;
    sb->fm_queue[slot].reg = (UInt16)((entry >> 8) & 0x1FF);
    sb->fm_queue[slot].value = (UInt8)entry;
    sb->fm_count++;
}

static void sb16_mixer_reset(emex64_sb16_t *sb)
{
    memset(sb->mixer, 0, sizeof(sb->mixer));
    sb->mixer[SB16_MIXER_MASTER_LEFT] = 0xC0;
    sb->mixer[SB16_MIXER_MASTER_RIGHT] = 0xC0;
    sb->mixer[SB16_MIXER_VOICE_LEFT] = 0xC0;
    sb->mixer[SB16_MIXER_VOICE_RIGHT] = 0xC0;
    sb->mixer[SB16_MIXER_FM_LEFT] = 0xC0;
    sb->mixer[SB16_MIXER_FM_RIGHT] = 0xC0;
    sb->mixer[SB16_MIXER_IRQ_SELECT] = 0x02;
    sb->mixer[SB16_MIXER_DMA_SELECT] = 0x22;
}

static void sb16_dsp_reset(emex64_sb16_t *sb)
{
    sb->cmd = 0;
    sb->arg_need = 0;
    sb->arg_have = 0;
    sb->reply_head = 0;
    sb->reply_count = 0;
    sb->test_reg = 0;
    sb->speaker = false;
    sb->rate = 11025;
    sb->block8 = 0;
    sb->playing = false;
    sb->paused = false;
    sb->autoinit = false;
    sb->block_bytes = 0;
    sb->block_left = 0;
    sb->dma_pos = 0;
    sb->irq_status = 0;
    sb->phase = 0;
    memset(sb->cur, 0, sizeof(sb->cur));
    memset(sb->next, 0, sizeof(sb->next));
}

static void sb16_reply(emex64_sb16_t *sb,
                       UInt8 value)
{
    if(sb->reply_count < SB16_OUT_BUFFER)
    {
        sb->reply[(sb->reply_head + sb->reply_count) % SB16_OUT_BUFFER] = value;
        sb->reply_count++;
    }
}

static inline UInt32 sb16_frame_bytes(const emex64_sb16_t *sb)
{
    return (sb->bits16 ? 2u : 1u) * (sb->stereo ? 2u : 1u);
}

static void sb16_start(emex64_sb16_t *sb,
                       Boolean bits16,
                       Boolean stereo,
                       Boolean is_signed,
                       Boolean autoinit,
                       UInt32 block_bytes)
{
    sb->bits16 = bits16;
    sb->stereo = stereo;
    sb->is_signed = is_signed;
    sb->autoinit = autoinit;
    sb->block_bytes = block_bytes;
    sb->block_left = block_bytes;
    sb->dma_pos = 0;
    sb->phase = 0;
    memset(sb->cur, 0, sizeof(sb->cur));
    memset(sb->next, 0, sizeof(sb->next));
    sb->paused = false;
    sb->playing = block_bytes != 0;
}

static UInt8 sb16_arg_count(UInt8 cmd)
{
    if(cmd >= 0xB0 && cmd <= 0xCF)
    {
        return 3;
    }
    switch(cmd)
    {
        case 0x10:
        case 0x40:
        case 0xE0:
        case 0xE4:
            return 1;
        case 0x14:
        case 0x41:
        case 0x42:
        case 0x48:
        case 0x80:
            return 2;
        default:
            return 0;
    }
}

static UInt8 sb16_command(emex64_sb16_t *sb)
{
    UInt8 cmd = sb->cmd;
    const UInt8 *a = sb->args;

    if(cmd >= 0xB0 && cmd <= 0xCF)
    {
        if(!(cmd & 0x08))
        {
            Boolean bits16 = cmd < 0xC0;
            UInt32 samples = ((UInt32)a[1] | ((UInt32)a[2] << 8)) + 1;
            sb16_start(sb, bits16, (a[0] & 0x20) != 0, (a[0] & 0x10) != 0, (cmd & 0x04) != 0, samples * (bits16 ? 2u : 1u));
        }
        return 0;
    }

    switch(cmd)
    {
        case 0x14:
        case 0x91:
            if(cmd == 0x14)
            {
                sb->block8 = ((UInt32)a[0] | ((UInt32)a[1] << 8)) + 1;
            }
            sb16_start(sb, false, false, false, false, sb->block8);
            break;
        case 0x1C:
        case 0x90:
            sb16_start(sb, false, false, false, true, sb->block8);
            break;
        case 0x40:
            if(a[0] != 0xFF)
            {
                sb->rate = 1000000u / (256u - a[0]);
            }
            break;
        case 0x41:
        case 0x42:
            sb->rate = ((UInt32)a[0] << 8) | a[1];
            if(sb->rate < 4000)
            {
                sb->rate = 4000;
            }
            if(sb->rate > 48000)
            {
                sb->rate = 48000;
            }
            break;
        case 0x48:
            sb->block8 = ((UInt32)a[0] | ((UInt32)a[1] << 8)) + 1;
            break;
        case 0xD0:
        case 0xD5:
            sb->paused = true;
            break;
        case 0xD4:
        case 0xD6:
            sb->paused = false;
            break;
        case 0xD1:
            sb->speaker = true;
            break;
        case 0xD3:
            sb->speaker = false;
            break;
        case 0xD8:
            sb16_reply(sb, sb->speaker ? 0xFF : 0x00);
            break;
        case 0xD9:
        case 0xDA:
            sb->autoinit = false;
            break;
        case 0xE0:
            sb16_reply(sb, (UInt8)~a[0]);
            break;
        case 0xE1:
            sb16_reply(sb, SB16_DSP_VERSION_MAJOR);
            sb16_reply(sb, SB16_DSP_VERSION_MINOR);
            break;
        case 0xE3:
        {
            static const char copyright[] = "COPYRIGHT (C) CREATIVE TECHNOLOGY LTD, 1992.";
            for(UInt32 i = 0; i < sizeof(copyright); i++)
            {
                sb16_reply(sb, (UInt8)copyright[i]);
            }
            break;
        }
        case 0xE4:
            sb->test_reg = a[0];
            break;
        case 0xE8:
            sb16_reply(sb, sb->test_reg);
            break;
        case 0xF2:
            return SB16_IRQ_8;
        case 0xF3:
            return SB16_IRQ_16;
        default:
            break;
    }
    return 0;
}

static Boolean sb16_fetch(emex64_sb16_t *sb,
                          SInt32 *frame)
{
    UInt32 bytes = sb16_frame_bytes(sb);
    if(sb->dma_size < bytes)
    {
        return false;
    }
    if(sb->dma_pos + bytes > sb->dma_size)
    {
        sb->dma_pos = 0;
    }

    const UInt8 *p = emex64_pcie_dma_map(&sb->function, sb->dma_addr + sb->dma_pos, bytes, false);
    if(p == NULL)
    {
        return false;
    }

    for(UInt32 c = 0; c < (sb->stereo ? 2u : 1u); c++)
    {
        SInt32 v;
        if(sb->bits16)
        {
            v = (SInt32)((UInt32)p[c * 2] | ((UInt32)p[c * 2 + 1] << 8));
            v = sb->is_signed ? (SInt16)v : v - 32768;
        }
        else
        {
            v = sb->is_signed ? (SInt8)p[c] * 256 : ((SInt32)p[c] - 128) * 256;
        }
        frame[c] = v;
    }
    if(!sb->stereo)
    {
        frame[1] = frame[0];
    }

    sb->dma_pos += bytes;
    if(sb->dma_pos >= sb->dma_size)
    {
        sb->dma_pos = 0;
    }
    return true;
}

static inline SInt32 sb16_volume(UInt8 reg)
{
    return (reg >> 3) + ((reg >> 3) != 0 ? 1 : 0);
}

static void sb16_render(void *context,
                        SInt16 *stereo,
                        UInt32 frames)
{
    emex64_sb16_t *sb = context;
    Boolean irq = false;

    pthread_mutex_lock(&sb->lock);

    UInt64 step = ((UInt64)sb->rate << 32) / EMEX64_SOUND_RATE;
    SInt32 gain[2] = {
        sb16_volume(sb->mixer[SB16_MIXER_MASTER_LEFT]) * sb16_volume(sb->mixer[SB16_MIXER_VOICE_LEFT]),
        sb16_volume(sb->mixer[SB16_MIXER_MASTER_RIGHT]) * sb16_volume(sb->mixer[SB16_MIXER_VOICE_RIGHT]),
    };

    SInt32 fm_gain[2] = {
        sb16_volume(sb->mixer[SB16_MIXER_MASTER_LEFT]) * sb16_volume(sb->mixer[SB16_MIXER_FM_LEFT]),
        sb16_volume(sb->mixer[SB16_MIXER_MASTER_RIGHT]) * sb16_volume(sb->mixer[SB16_MIXER_FM_RIGHT]),
    };

    for(UInt32 i = 0; i < frames; i++)
    {
        SInt64 fm[2] = { 0, 0 };
        if(sb->fm_used)
        {
            while(sb->fm_count > 0 && sb->fm_queue[sb->fm_head].at <= sb->fm_now)
            {
                sb16_fm_write(sb, sb->fm_queue[sb->fm_head].reg, sb->fm_queue[sb->fm_head].value);
                sb->fm_head = (sb->fm_head + 1) % SB16_FM_QUEUE_SIZE;
                sb->fm_count--;
            }
            int16_t sample[2];
            OPL3_GenerateResampled(&sb->fm, sample);
            fm[0] = ((SInt64)sample[0] * fm_gain[0]) >> 10;
            fm[1] = ((SInt64)sample[1] * fm_gain[1]) >> 10;
        }
        sb->fm_now++;

        if(!sb->playing || sb->paused)
        {
            for(UInt32 c = 0; c < 2; c++)
            {
                stereo[i * 2 + c] = (SInt16)(fm[c] > 32767 ? 32767 : fm[c] < -32768 ? -32768 : fm[c]);
            }
            continue;
        }

        while(sb->phase >= (1ULL << 32) && sb->playing)
        {
            sb->phase -= 1ULL << 32;
            sb->cur[0] = sb->next[0];
            sb->cur[1] = sb->next[1];
            if(!sb16_fetch(sb, sb->next))
            {
                sb->playing = false;
                sb->next[0] = sb->next[1] = 0;
                break;
            }

            UInt32 bytes = sb16_frame_bytes(sb);
            if(sb->block_left <= bytes)
            {
                sb->irq_status |= sb->bits16 ? SB16_IRQ_16 : SB16_IRQ_8;
                irq = true;
                if(sb->autoinit)
                {
                    sb->block_left = sb->block_bytes;
                }
                else
                {
                    sb->playing = false;
                }
            }
            else
            {
                sb->block_left -= bytes;
            }
        }

        SInt64 frac = (SInt64)(sb->phase >> 16) & 0xFFFF;
        for(UInt32 c = 0; c < 2; c++)
        {
            SInt64 v = sb->cur[c] + (((SInt64)(sb->next[c] - sb->cur[c]) * frac) >> 16);
            v = ((v * gain[c]) >> 10) + fm[c];
            if(v > 32767)
            {
                v = 32767;
            }
            if(v < -32768)
            {
                v = -32768;
            }
            stereo[i * 2 + c] = (SInt16)v;
        }
        sb->phase += step;
    }

    pthread_mutex_unlock(&sb->lock);

    if(irq)
    {
        emex64_pcie_raise_irq(&sb->function);
    }
}

static UInt64 sb16_bar_read(emex64_pcie_function_t *function,
                            UInt8 bar,
                            UInt64 offset)
{
    emex64_sb16_t *sb = function->context;
    UInt64 value = 0;

    pthread_mutex_lock(&sb->lock);
    switch(offset)
    {
        case SB16_REG_FM_ADDR:
        case SB16_REG_ADLIB_ADDR:
            value = sb->fm_status;
            break;
        case SB16_REG_FM_QUEUED:
        {
            UInt64 now = sb->fm_now * 1000000ULL;
            value = (sb->fm_count > 0 && sb->fm_due > now) ? (sb->fm_due - now) / EMEX64_SOUND_RATE : 0;
            break;
        }
        case SB16_REG_MIXER_ADDR:
            value = sb->mixer_addr;
            break;
        case SB16_REG_MIXER_DATA:
            value = sb->mixer_addr == SB16_MIXER_IRQ_STATUS ? sb->irq_status : sb->mixer[sb->mixer_addr];
            break;
        case SB16_REG_READ_DATA:
            if(sb->reply_count > 0)
            {
                value = sb->reply[sb->reply_head];
                sb->reply_head = (UInt8)((sb->reply_head + 1) % SB16_OUT_BUFFER);
                sb->reply_count--;
            }
            else
            {
                value = 0xFF;
            }
            break;
        case SB16_REG_WRITE:
            value = 0x7F;
            break;
        case SB16_REG_READ_STATUS:
            value = sb->reply_count > 0 ? 0xFF : 0x7F;
            sb->irq_status &= (UInt8)~SB16_IRQ_8;
            break;
        case SB16_REG_ACK16:
            value = 0xFF;
            sb->irq_status &= (UInt8)~SB16_IRQ_16;
            break;
        case SB16_REG_DMA_ADDR:
            value = sb->dma_addr;
            break;
        case SB16_REG_DMA_SIZE:
            value = sb->dma_size;
            break;
        case SB16_REG_DMA_POS:
            value = sb->dma_pos;
            break;
        default:
            value = 0xFF;
            break;
    }
    pthread_mutex_unlock(&sb->lock);
    return value;
}

static void sb16_bar_write(emex64_pcie_function_t *function,
                           UInt8 bar,
                           UInt64 offset,
                           UInt64 value)
{
    emex64_sb16_t *sb = function->context;
    UInt8 irq = 0;
    Boolean open = false;

    pthread_mutex_lock(&sb->lock);
    switch(offset)
    {
        case SB16_REG_FM_ADDR:
        case SB16_REG_ADLIB_ADDR:
            sb->fm_addr[0] = (UInt8)value;
            break;
        case SB16_REG_FM_ADDR2:
            sb->fm_addr[1] = (UInt16)(0x100 | (UInt8)value);
            break;
        case SB16_REG_FM_DATA:
        case SB16_REG_ADLIB_DATA:
            sb16_fm_write(sb, sb->fm_addr[0], (UInt8)value);
            open = sb->out == NULL && !sb->out_tried;
            break;
        case SB16_REG_FM_DATA2:
            sb16_fm_write(sb, sb->fm_addr[1], (UInt8)value);
            open = sb->out == NULL && !sb->out_tried;
            break;
        case SB16_REG_FM_QUEUE:
            sb16_fm_enqueue(sb, value);
            open = sb->out == NULL && !sb->out_tried;
            break;
        case SB16_REG_FM_QUEUED:
            sb->fm_head = 0;
            sb->fm_count = 0;
            break;
        case SB16_REG_MIXER_ADDR:
            sb->mixer_addr = (UInt8)value;
            break;
        case SB16_REG_MIXER_DATA:
            switch(sb->mixer_addr)
            {
                case SB16_MIXER_RESET:
                    sb16_mixer_reset(sb);
                    break;
                case 0x04:
                    sb->mixer[SB16_MIXER_VOICE_LEFT] = (UInt8)(value & 0xF0);
                    sb->mixer[SB16_MIXER_VOICE_RIGHT] = (UInt8)((value & 0x0F) << 4);
                    break;
                case 0x22:
                    sb->mixer[SB16_MIXER_MASTER_LEFT] = (UInt8)(value & 0xF0);
                    sb->mixer[SB16_MIXER_MASTER_RIGHT] = (UInt8)((value & 0x0F) << 4);
                    break;
                case SB16_MIXER_IRQ_STATUS:
                    break;
                default:
                    sb->mixer[sb->mixer_addr] = (UInt8)value;
                    break;
            }
            break;
        case SB16_REG_RESET:
            if((sb->reset_reg & 1) && !(value & 1))
            {
                sb16_dsp_reset(sb);
                sb16_reply(sb, 0xAA);
            }
            sb->reset_reg = (UInt8)value;
            break;
        case SB16_REG_WRITE:
            if(sb->arg_need == 0)
            {
                sb->cmd = (UInt8)value;
                sb->arg_have = 0;
                sb->arg_need = sb16_arg_count(sb->cmd);
            }
            else
            {
                sb->args[sb->arg_have++] = (UInt8)value;
                sb->arg_need--;
            }
            if(sb->arg_need == 0)
            {
                irq = sb16_command(sb);
                sb->irq_status |= irq;
                open = open || (sb->playing && sb->out == NULL && !sb->out_tried);
            }
            break;
        case SB16_REG_DMA_ADDR:
            sb->dma_addr = value;
            sb->dma_pos = 0;
            break;
        case SB16_REG_DMA_SIZE:
            sb->dma_size = value;
            sb->dma_pos = 0;
            break;
        default:
            break;
    }
    pthread_mutex_unlock(&sb->lock);

    if(open)
    {
        sb->out_tried = true;
        sb->out = emex64_sound_out_open(sb16_render, sb, sb->wav_path);
    }
    if(irq != 0)
    {
        emex64_pcie_raise_irq(&sb->function);
    }
}

static const emex64_pcie_ops_t sb16_ops = {
    .bar_read = sb16_bar_read,
    .bar_write = sb16_bar_write,
};

emex64_sb16_t *emex64_sb16_alloc(emex64_pcie_t *pcie,
                                 UInt8 dev,
                                 const E64MachineOptions *options)
{
    emex64_sb16_t *sb = calloc(1, sizeof(emex64_sb16_t));
    if(sb == NULL)
    {
        return NULL;
    }
    pthread_mutex_init(&sb->lock, NULL);
    sb->wav_path = options->soundOptions.wavPath;

    sb->function.vendor = PCIE_VENDOR_EMEXLABS;
    sb->function.device = SB16_PCI_DEVICE_ID;
    sb->function.class_code = SB16_PCI_CLASS;
    sb->function.revision = 1;
    sb->function.bar_size[0] = SB16_BAR0_SIZE;
    sb->function.ops = &sb16_ops;
    sb->function.context = sb;
    sb16_dsp_reset(sb);
    sb16_mixer_reset(sb);
    sb16_fm_reset(sb);

    if(!emex64_pcie_attach(pcie, &sb->function, dev, 0))
    {
        pthread_mutex_destroy(&sb->lock);
        free(sb);
        return NULL;
    }
    return sb;
}

void emex64_sb16_dealloc(emex64_sb16_t *sb)
{
    if(sb == NULL)
    {
        return;
    }
    emex64_sound_out_close(sb->out);
    pthread_mutex_destroy(&sb->lock);
    free(sb);
}

void emex64_sb16_reset(emex64_sb16_t *sb)
{
    if(sb == NULL)
    {
        return;
    }
    pthread_mutex_lock(&sb->lock);
    sb16_dsp_reset(sb);
    sb16_mixer_reset(sb);
    sb16_fm_reset(sb);
    sb->reset_reg = 0;
    sb->mixer_addr = 0;
    sb->dma_addr = 0;
    sb->dma_size = 0;
    pthread_mutex_unlock(&sb->lock);
}

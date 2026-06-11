/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EMEX64VM_DEVICE_AC97_H
#define EMEX64VM_DEVICE_AC97_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>

#include <emex64lib/vm/device/base.h>

#define EMEX64_AC97_SIZE        EMEX64_PAGE_SIZE

#define AC97_CODEC_RESET            0x000
#define AC97_CODEC_MASTER_VOL       0x002
#define AC97_CODEC_HEADPHONE_VOL    0x004
#define AC97_CODEC_PCM_OUT_VOL      0x018
#define AC97_CODEC_RECORD_SELECT    0x01A
#define AC97_CODEC_RECORD_GAIN      0x01C
#define AC97_CODEC_GENERAL_PURPOSE  0x020
#define AC97_CODEC_POWERDOWN        0x026
#define AC97_CODEC_EXT_AUDIO_ID     0x028
#define AC97_CODEC_EXT_AUDIO_CTRL   0x02A
#define AC97_CODEC_PCM_DAC_RATE     0x02C
#define AC97_CODEC_PCM_ADC_RATE     0x032
#define AC97_CODEC_VENDOR_ID1       0x07C
#define AC97_CODEC_VENDOR_ID2       0x07E

#define AC97_CODEC_REG_COUNT        64

#define AC97_PWR_ADC_READY          (1 << 0)
#define AC97_PWR_DAC_READY          (1 << 1)
#define AC97_PWR_ANL_READY          (1 << 2)
#define AC97_PWR_VREF_UP            (1 << 3)
#define AC97_PWR_PR0                (1 << 8)    /* ADC power off */
#define AC97_PWR_PR1                (1 << 9)    /* DAC power off */

#define AC97_EXT_VRA                (1 << 0)    /* variable rate audio */

#define AC97_BM_BASE                0x080

#define AC97_BM_PCMOUT_BDL_BASE     0x080   /* R/W  qword  guest phys addr of BDL array */
#define AC97_BM_PCMOUT_CIV          0x088   /* RO   byte   current index value          */
#define AC97_BM_PCMOUT_LVI          0x089   /* R/W  byte   last valid index             */
#define AC97_BM_PCMOUT_STATUS       0x08A   /* R/W  word   channel status (W1C bits)    */
#define AC97_BM_PCMOUT_PICB         0x08C   /* RO   word   samples left in current buf  */
#define AC97_BM_PCMOUT_PIV          0x08E   /* RO   byte   prefetched index value       */
#define AC97_BM_PCMOUT_CTRL         0x08F   /* R/W  byte   channel control              */
#define AC97_BM_GLOB_CTRL           0x0B8   /* R/W  dword  global control               */
#define AC97_BM_GLOB_STATUS         0x0BC   /* R/W  dword  global status                */

#define AC97_BM_STATUS_DCH          (1 << 0)    /* DMA halted                  */
#define AC97_BM_STATUS_CELV         (1 << 1)    /* current index == last valid */
#define AC97_BM_STATUS_LVBCI        (1 << 2)    /* last valid buf completed    */
#define AC97_BM_STATUS_BCIS         (1 << 3)    /* buffer completion interrupt */
#define AC97_BM_STATUS_FIFOE        (1 << 4)    

#define AC97_BM_CTRL_RUN            (1 << 0)    /* start / pause DMA           */
#define AC97_BM_CTRL_RESET          (1 << 1)    /* reset channel               */
#define AC97_BM_CTRL_LVBIE          (1 << 2)    /* LVI interrupt enable        */
#define AC97_BM_CTRL_FEIE           (1 << 3)    /* FIFO error interrupt enable */
#define AC97_BM_CTRL_IOCE           (1 << 4)    /* IOC interrupt enable        */

#define AC97_BM_GLOB_CTRL_GIE       (1 << 0)    /* global interrupt enable     */
#define AC97_BM_GLOB_CTRL_COLD_RST  (1 << 1)    
#define AC97_BM_GLOB_CTRL_WARM_RST  (1 << 2)    

#define AC97_BDL_MAX_ENTRIES        32
#define AC97_BDL_FLAG_IOC           (1 << 15)   /* interrupt on completion     */
#define AC97_BDL_FLAG_BUP           (1 << 14)   /* repeat on underrun          */

typedef struct {
    uint32_t base;    /* guest-physical byte address of PCM data */
    uint16_t length;  /* number of 16-bit samples (per channel)  */
    uint16_t flags;   /* AC97_BDL_FLAG_* bits                    */
} emex64_ac97_bdle_t;

#define AC97_DEFAULT_SAMPLE_RATE    48000
#define AC97_DEFAULT_MASTER_VOL     0x0000
#define AC97_DEFAULT_PCM_VOL        0x0000
#define AC97_VENDOR_ID1             0x454D  /* 'EM' */
#define AC97_VENDOR_ID2             0x3634  /* '64' */

/* PCM ring: 32 KiB ≈ 170 ms of stereo 16-bit at 48 kHz.  Must be power-of-two. */
#define AC97_RING_SIZE              (1 << 15)
#define AC97_RING_MASK              (AC97_RING_SIZE - 1)

typedef struct emex64_core    emex64_core_t;
typedef struct emex64_machine emex64_machine_t;

typedef struct emex64_ac97 {
    uint16_t codec_regs[AC97_CODEC_REG_COUNT];

    uint64_t bm_bdl_base;
    uint8_t  bm_civ;        /* current index value                        */
    uint8_t  bm_civ_next;   /* pending CIV advance, committed on BCIS ack */
    uint8_t  bm_lvi;        /* last valid index                           */
    uint16_t bm_status;
    uint16_t bm_picb;
    uint8_t  bm_piv;
    uint8_t  bm_ctrl;

    uint32_t bm_glob_ctrl;
    uint32_t bm_glob_status;

    emex64_ac97_bdle_t bdl[AC97_BDL_MAX_ENTRIES];
    uint32_t           samples_remaining;

    /* lock-free PCM ring; head owned by VM thread, tail by audio thread */
    uint8_t          ring[AC97_RING_SIZE];
    _Atomic uint32_t ring_head;
    _Atomic uint32_t ring_tail;

#if EMEX64VM_DEVICE_AUDIO && (defined(__linux__) || defined(__APPLE__))
    pthread_t        audio_thread;
    _Atomic bool     audio_stop;
    bool             audio_enabled;
    _Atomic uint32_t audio_rate;         /* current DAC sample rate (Hz)      */
    _Atomic bool     audio_rate_changed; /* set by write handler on rate change */
#endif

    emex64_machine_t *machine;
} emex64_ac97_t;

emex64_ac97_t *emex64_ac97_alloc(emex64_machine_t *machine, bool install);
void           emex64_ac97_dealloc(emex64_ac97_t *ac97);

uint64_t emex64_ac97_read (emex64_core_t *core, void *device, uint64_t offset, int size);
void     emex64_ac97_write(emex64_core_t *core, void *device, uint64_t offset, uint64_t value, int size);

/* Advances the BDL DMA engine; call each VM execute loop iteration. */
void emex64_ac97_tick(emex64_ac97_t *ac97, emex64_core_t *core);

#endif /* EMEX64VM_DEVICE_AC97_H */

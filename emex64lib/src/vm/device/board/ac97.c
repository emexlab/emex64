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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#if EMEX64VM_DEVICE_AUDIO && defined(__linux__)
#include <time.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#endif /* EMEX64VM_DEVICE_AUDIO && __linux__ */

#if EMEX64VM_DEVICE_AUDIO && defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>
#endif /* EMEX64VM_DEVICE_AUDIO && __APPLE__ */

#include <emex64lib/vm/machine.h>
#include <emex64lib/vm/memory.h>
#include <emex64lib/vm/device/board/ac97.h>
#include <emex64lib/vm/device/internal/controller/ic.h>


// Ac97 driver for emex64 VM 


static void ac97_codec_reset(emex64_ac97_t *ac97)
{
    memset(ac97->codec_regs, 0, sizeof(ac97->codec_regs));

    ac97->codec_regs[AC97_CODEC_MASTER_VOL    >> 1] = AC97_DEFAULT_MASTER_VOL;
    ac97->codec_regs[AC97_CODEC_HEADPHONE_VOL >> 1] = AC97_DEFAULT_MASTER_VOL;
    ac97->codec_regs[AC97_CODEC_PCM_OUT_VOL   >> 1] = AC97_DEFAULT_PCM_VOL;
    ac97->codec_regs[AC97_CODEC_PCM_DAC_RATE  >> 1] = (uint16_t)AC97_DEFAULT_SAMPLE_RATE;
    ac97->codec_regs[AC97_CODEC_PCM_ADC_RATE  >> 1] = (uint16_t)AC97_DEFAULT_SAMPLE_RATE;
    ac97->codec_regs[AC97_CODEC_POWERDOWN     >> 1] = AC97_PWR_ADC_READY | AC97_PWR_DAC_READY |
                                                      AC97_PWR_ANL_READY | AC97_PWR_VREF_UP;
    ac97->codec_regs[AC97_CODEC_EXT_AUDIO_ID  >> 1] = AC97_EXT_VRA;
    ac97->codec_regs[AC97_CODEC_EXT_AUDIO_CTRL >> 1] = 0x0000;
    ac97->codec_regs[AC97_CODEC_VENDOR_ID1    >> 1] = AC97_VENDOR_ID1;
    ac97->codec_regs[AC97_CODEC_VENDOR_ID2    >> 1] = AC97_VENDOR_ID2;
}

static void ac97_bm_channel_reset(emex64_ac97_t *ac97)
{
    ac97->bm_bdl_base       = 0;
    ac97->bm_civ            = 0;
    ac97->bm_civ_next       = 0;
    ac97->bm_lvi            = 0;
    ac97->bm_status         = AC97_BM_STATUS_DCH;
    ac97->bm_picb           = 0;
    ac97->bm_piv            = 0;
    ac97->bm_ctrl          &= ~(AC97_BM_CTRL_RUN | AC97_BM_CTRL_RESET);
    ac97->samples_remaining = 0;
    memset(ac97->bdl, 0, sizeof(ac97->bdl));
}

// Linux:  PulseAudio simple API.
// macOS:  AudioQueue with a fill callback.


#if EMEX64VM_DEVICE_AUDIO && defined(__linux__)

/* 2048 frames × 4 bytes = 8192 bytes ≈ 42 ms at 48 kHz */
#define AC97_PA_CHUNK_FRAMES  2048u
#define AC97_PA_CHUNK_BYTES   (AC97_PA_CHUNK_FRAMES * 4u)

/* empty-ring polls before feeding silence; ~100 ms at 1 ms per iteration */
#define AC97_UNDERRUN_THRESHOLD  100u

static const int16_t ac97_silence[AC97_PA_CHUNK_FRAMES * 2] = { 0 };

static void *ac97_audio_thread(void *arg)
{
    emex64_ac97_t *ac97 = (emex64_ac97_t *)arg;

    pa_simple     *pa           = NULL;
    uint32_t       current_rate = 0;
    int            pa_err       = 0;
    uint32_t       empty_count  = 0;
    uint8_t        chunk[AC97_PA_CHUNK_BYTES];

    while(!atomic_load_explicit(&ac97->audio_stop, memory_order_acquire))
    {
        uint32_t new_rate   = atomic_load_explicit(&ac97->audio_rate, memory_order_relaxed);
        bool     rate_changed = atomic_load_explicit(&ac97->audio_rate_changed, memory_order_acquire);

        if(new_rate == 0)
            new_rate = AC97_DEFAULT_SAMPLE_RATE;

        if(pa == NULL || rate_changed || new_rate != current_rate)
        {
            atomic_store_explicit(&ac97->audio_rate_changed, false, memory_order_release);

            if(pa != NULL)
            {
                pa_simple_free(pa);
                pa = NULL;
            }

            current_rate = new_rate;
            empty_count  = 0;

            pa_sample_spec ss = {
                .format   = PA_SAMPLE_S16LE,
                .rate     = current_rate,
                .channels = 2,
            };
            pa_buffer_attr ba = {
                .maxlength = (uint32_t)-1,
                .tlength   = pa_usec_to_bytes(40000, &ss),  /* 40 ms — tight enough to stay in sync with video */
                .prebuf    = 0,                              /* start playback immediately, no prebuffering */
                .minreq    = (uint32_t)-1,
                .fragsize  = (uint32_t)-1,
            };

            pa = pa_simple_new(NULL, "emex64vm", PA_STREAM_PLAYBACK,
                               NULL, "AC97 PCM out", &ss, NULL, &ba, &pa_err);

            if(pa == NULL)
            {
                struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000L };
                nanosleep(&ts, NULL);
                continue;
            }
        }

        uint32_t head  = atomic_load_explicit(&ac97->ring_head, memory_order_acquire);
        uint32_t tail  = atomic_load_explicit(&ac97->ring_tail, memory_order_relaxed);
        uint32_t avail = (head - tail) & AC97_RING_MASK;

        if(avail < AC97_PA_CHUNK_BYTES)
        {
            empty_count++;

            if(empty_count >= AC97_UNDERRUN_THRESHOLD)
            {
                /* ring has been dry long enough — feed silence to keep the stream alive */
                pa_simple_write(pa, ac97_silence, sizeof(ac97_silence), &pa_err);
                empty_count = 0;
            }
            else
            {
                struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000L }; /* 1 ms */
                nanosleep(&ts, NULL);
            }
            continue;
        }

        empty_count = 0;

        for(uint32_t i = 0; i < AC97_PA_CHUNK_BYTES; i++)
            chunk[i] = ac97->ring[(tail + i) & AC97_RING_MASK];

        atomic_store_explicit(&ac97->ring_tail,
                              (tail + AC97_PA_CHUNK_BYTES) & AC97_RING_MASK,
                              memory_order_release);

        pa_simple_write(pa, chunk, AC97_PA_CHUNK_BYTES, &pa_err);
    }

    if(pa != NULL)
    {
        pa_simple_drain(pa, NULL);
        pa_simple_free(pa);
    }

    return NULL;
}

#endif /* EMEX64VM_DEVICE_AUDIO && __linux__ */

// macOS AudioQueue it pulls samples via fill callbacks 
#if EMEX64VM_DEVICE_AUDIO && defined(__APPLE__)

#define AC97_AQ_NBUFFERS   3u
#define AC97_AQ_BUF_FRAMES 2048u
#define AC97_AQ_BUF_BYTES  (AC97_AQ_BUF_FRAMES * 4u)

typedef struct {
    emex64_ac97_t  *ac97;
    AudioQueueRef   queue;
    AudioQueueBufferRef bufs[AC97_AQ_NBUFFERS];
} ac97_aq_ctx_t;

static void ac97_aq_callback(void *user_data,
                              AudioQueueRef        queue,
                              AudioQueueBufferRef  buf)
{
    ac97_aq_ctx_t *ctx  = (ac97_aq_ctx_t *)user_data;
    emex64_ac97_t *ac97 = ctx->ac97;

    uint32_t head    = atomic_load_explicit(&ac97->ring_head, memory_order_acquire);
    uint32_t tail    = atomic_load_explicit(&ac97->ring_tail, memory_order_relaxed);
    uint32_t avail   = (head - tail) & AC97_RING_MASK;
    uint32_t consume = (avail < AC97_AQ_BUF_BYTES) ? avail : AC97_AQ_BUF_BYTES;
    uint8_t *dst     = (uint8_t *)buf->mAudioData;

    for(uint32_t i = 0; i < consume; i++)
        dst[i] = ac97->ring[(tail + i) & AC97_RING_MASK];

    if(consume < AC97_AQ_BUF_BYTES)
        memset(dst + consume, 0, AC97_AQ_BUF_BYTES - consume);

    atomic_store_explicit(&ac97->ring_tail,
                          (tail + consume) & AC97_RING_MASK,
                          memory_order_release);

    buf->mAudioDataByteSize = AC97_AQ_BUF_BYTES;
    AudioQueueEnqueueBuffer(queue, buf, 0, NULL);
}

static void *ac97_audio_thread(void *arg)
{
    emex64_ac97_t  *ac97 = (emex64_ac97_t *)arg;
    ac97_aq_ctx_t   ctx  = { .ac97 = ac97 };
    uint32_t current_rate = 0;

    while(!atomic_load_explicit(&ac97->audio_stop, memory_order_acquire))
    {
        uint32_t new_rate = atomic_load_explicit(&ac97->audio_rate, memory_order_relaxed);
        bool     rate_chg = atomic_load_explicit(&ac97->audio_rate_changed, memory_order_acquire);

        if(new_rate == 0)
            new_rate = AC97_DEFAULT_SAMPLE_RATE;

        if(ctx.queue == NULL || rate_chg || new_rate != current_rate)
        {
            atomic_store_explicit(&ac97->audio_rate_changed, false, memory_order_release);

            if(ctx.queue != NULL)
            {
                AudioQueueStop(ctx.queue, true);
                AudioQueueDispose(ctx.queue, true);
                ctx.queue = NULL;
            }

            current_rate = new_rate;

            AudioStreamBasicDescription fmt = {
                .mSampleRate       = (Float64)current_rate,
                .mFormatID         = kAudioFormatLinearPCM,
                .mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger
                                   | kLinearPCMFormatFlagIsPacked,
                .mBytesPerPacket   = 4,
                .mFramesPerPacket  = 1,
                .mBytesPerFrame    = 4,
                .mChannelsPerFrame = 2,
                .mBitsPerChannel   = 16,
            };

            if(AudioQueueNewOutput(&fmt, ac97_aq_callback, &ctx,
                                   NULL, NULL, 0, &ctx.queue) != noErr)
            {
                struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000L };
                nanosleep(&ts, NULL);
                continue;
            }

            for(unsigned i = 0; i < AC97_AQ_NBUFFERS; i++)
            {
                AudioQueueAllocateBuffer(ctx.queue, AC97_AQ_BUF_BYTES, &ctx.bufs[i]);
                ctx.bufs[i]->mAudioDataByteSize = AC97_AQ_BUF_BYTES;
                memset(ctx.bufs[i]->mAudioData, 0, AC97_AQ_BUF_BYTES);
                AudioQueueEnqueueBuffer(ctx.queue, ctx.bufs[i], 0, NULL);
            }

            AudioQueueStart(ctx.queue, NULL);
        }

        /* callback drives everything; just yield */
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 10000000L }; /* 10 ms */
        nanosleep(&ts, NULL);
    }

    if(ctx.queue != NULL)
    {
        AudioQueueStop(ctx.queue, true);
        AudioQueueDispose(ctx.queue, true);
    }

    return NULL;
}

#endif /* EMEX64VM_DEVICE_AUDIO && __APPLE__ */

static void *ac97_dma_thread(void *arg);

emex64_ac97_t *emex64_ac97_alloc(emex64_machine_t *machine, bool install)
{
    emex64_ac97_t *ac97 = calloc(1, sizeof(emex64_ac97_t));
    if(ac97 == NULL)
        return NULL;

    ac97->machine = machine;

    if(!emex64_mmio_register(machine->mmio_bus, EMEX64_AC97_BASE, EMEX64_AC97_SIZE,
                             ac97, emex64_ac97_read, emex64_ac97_write))
    {
        free(ac97);
        return NULL;
    }

    ac97_codec_reset(ac97);
    ac97->bm_status      = AC97_BM_STATUS_DCH;
    ac97->bm_glob_ctrl   = 0;
    ac97->bm_glob_status = 0;

    pthread_mutex_init(&ac97->dma_mutex, NULL);
    pthread_cond_init(&ac97->dma_cond, NULL);
    atomic_store_explicit(&ac97->dma_stop, false, memory_order_relaxed);
    ac97->dma_enabled = true;
    pthread_create(&ac97->dma_thread, NULL, ac97_dma_thread, ac97);

#if EMEX64VM_DEVICE_AUDIO && (defined(__linux__) || defined(__APPLE__))
    atomic_store_explicit(&ac97->audio_rate,         (uint32_t)AC97_DEFAULT_SAMPLE_RATE, memory_order_relaxed);
    atomic_store_explicit(&ac97->audio_rate_changed, false, memory_order_relaxed);
    atomic_store_explicit(&ac97->audio_stop,         false, memory_order_relaxed);

    if(install)
    {
        ac97->audio_enabled = true;
        pthread_create(&ac97->audio_thread, NULL, ac97_audio_thread, ac97);
    }
#endif

    return ac97;
}

void emex64_ac97_dealloc(emex64_ac97_t *ac97)
{
    if(ac97->dma_enabled)
    {
        atomic_store_explicit(&ac97->dma_stop, true, memory_order_release);
        pthread_mutex_lock(&ac97->dma_mutex);
        pthread_cond_signal(&ac97->dma_cond);
        pthread_mutex_unlock(&ac97->dma_mutex);
        pthread_join(ac97->dma_thread, NULL);
        pthread_cond_destroy(&ac97->dma_cond);
        pthread_mutex_destroy(&ac97->dma_mutex);
    }

#if EMEX64VM_DEVICE_AUDIO && (defined(__linux__) || defined(__APPLE__))
    if(ac97->audio_enabled)
    {
        atomic_store_explicit(&ac97->audio_stop, true, memory_order_release);
        pthread_join(ac97->audio_thread, NULL);
    }
#endif

    free(ac97);
}

uint64_t emex64_ac97_read(emex64_core_t *core, void *device, uint64_t offset, int size)
{
    (void)core;
    (void)size;

    emex64_ac97_t *ac97 = (emex64_ac97_t *)device;

    if(offset < AC97_BM_BASE)
    {
        uint64_t reg_idx = (offset & ~1ULL) >> 1;
        if(reg_idx >= AC97_CODEC_REG_COUNT)
            return 0;

        uint16_t val = ac97->codec_regs[reg_idx];
        return (offset & 1) ? (val >> 8) & 0xFF : val;
    }

    switch(offset)
    {
        case AC97_BM_PCMOUT_BDL_BASE: return ac97->bm_bdl_base;
        case AC97_BM_PCMOUT_CIV:      return ac97->bm_civ;
        case AC97_BM_PCMOUT_LVI:      return ac97->bm_lvi;
        case AC97_BM_PCMOUT_STATUS:   return ac97->bm_status;
        case AC97_BM_PCMOUT_PICB:     return ac97->bm_picb;
        case AC97_BM_PCMOUT_PIV:      return ac97->bm_piv;
        case AC97_BM_PCMOUT_CTRL:     return ac97->bm_ctrl;
        case AC97_BM_GLOB_CTRL:       return ac97->bm_glob_ctrl;
        case AC97_BM_GLOB_STATUS:     return ac97->bm_glob_status;
        default:                      return 0;
    }
}

void emex64_ac97_write(emex64_core_t *core, void *device, uint64_t offset, uint64_t value, int size)
{
    (void)core;
    (void)size;

    emex64_ac97_t *ac97 = (emex64_ac97_t *)device;

    if(offset < AC97_BM_BASE)
    {
        uint64_t reg_idx = (offset & ~1ULL) >> 1;
        if(reg_idx >= AC97_CODEC_REG_COUNT)
            return;

        if(offset == AC97_CODEC_RESET)         { ac97_codec_reset(ac97); return; }
        if(offset == AC97_CODEC_VENDOR_ID1 ||
           offset == AC97_CODEC_VENDOR_ID2 ||
           offset == AC97_CODEC_EXT_AUDIO_ID)  { return; }

        if(offset & 1)
            ac97->codec_regs[reg_idx] = (ac97->codec_regs[reg_idx] & 0x00FF) | ((uint16_t)(value & 0xFF) << 8);
        else
            ac97->codec_regs[reg_idx] = (uint16_t)(value & 0xFFFF);

#if EMEX64VM_DEVICE_AUDIO && (defined(__linux__) || defined(__APPLE__))
        // notify the audio thread when the DAC sample rate changes //
        if((offset & ~1ULL) == AC97_CODEC_PCM_DAC_RATE && !(offset & 1))
        {
            uint32_t new_rate = (uint32_t)ac97->codec_regs[AC97_CODEC_PCM_DAC_RATE >> 1];
            if(new_rate == 0)
                new_rate = AC97_DEFAULT_SAMPLE_RATE;
            atomic_store_explicit(&ac97->audio_rate, new_rate, memory_order_relaxed);
            atomic_store_explicit(&ac97->audio_rate_changed, true, memory_order_release);
        }
#endif /* EMEX64VM_DEVICE_AUDIO */

        return;
    }

    switch(offset)
    {
        case AC97_BM_PCMOUT_BDL_BASE:
            if(ac97->bm_status & AC97_BM_STATUS_DCH)
                ac97->bm_bdl_base = value;
            break;

        case AC97_BM_PCMOUT_LVI:
            ac97->bm_lvi = (uint8_t)(value & 0x1F);
            if((ac97->bm_status & AC97_BM_STATUS_DCH) && (ac97->bm_ctrl & AC97_BM_CTRL_RUN))
                ac97->bm_status &= ~AC97_BM_STATUS_DCH;
            pthread_mutex_lock(&ac97->dma_mutex);
            pthread_cond_signal(&ac97->dma_cond);
            pthread_mutex_unlock(&ac97->dma_mutex);
            break;

        case AC97_BM_PCMOUT_STATUS:
            ac97->bm_status &= ~((uint16_t)(value & 0xFFFF) &
                                 (AC97_BM_STATUS_LVBCI | AC97_BM_STATUS_BCIS | AC97_BM_STATUS_FIFOE));
            if((value & AC97_BM_STATUS_BCIS) && !(ac97->bm_status & AC97_BM_STATUS_BCIS))
            {
                ac97->bm_civ = ac97->bm_civ_next;
                pthread_mutex_lock(&ac97->dma_mutex);
                pthread_cond_signal(&ac97->dma_cond);
                pthread_mutex_unlock(&ac97->dma_mutex);
            }
            break;

        case AC97_BM_PCMOUT_CTRL:
        {
            uint8_t ctrl = (uint8_t)(value & 0xFF);
            if(ctrl & AC97_BM_CTRL_RESET)
            {
                ac97_bm_channel_reset(ac97);
                break;
            }
            ac97->bm_ctrl = ctrl;
            if(ctrl & AC97_BM_CTRL_RUN)
            {
                ac97->bm_status &= ~AC97_BM_STATUS_DCH;
                pthread_mutex_lock(&ac97->dma_mutex);
                pthread_cond_signal(&ac97->dma_cond);
                pthread_mutex_unlock(&ac97->dma_mutex);
            }
            else
            {
                ac97->bm_status |= AC97_BM_STATUS_DCH;
            }
            break;
        }

        case AC97_BM_GLOB_CTRL:
        {
            uint32_t ctrl = (uint32_t)(value & 0xFFFFFFFF);
            if(ctrl & AC97_BM_GLOB_CTRL_COLD_RST)
            {
                ac97_codec_reset(ac97);
                ac97_bm_channel_reset(ac97);
                ac97->bm_glob_ctrl   = 0;
                ac97->bm_glob_status = 0;
                break;
            }
            if(ctrl & AC97_BM_GLOB_CTRL_WARM_RST)
            {
                ac97_codec_reset(ac97);
                break;
            }
            ac97->bm_glob_ctrl = ctrl;
            break;
        }

        case AC97_BM_GLOB_STATUS:
            ac97->bm_glob_status &= ~(uint32_t)(value & 0xFFFFFFFF);
            break;

        default:
            break;
    }
}

static bool ac97_fetch_bdle(emex64_ac97_t *ac97, uint8_t index)
{
    uint64_t entry_addr = ac97->bm_bdl_base + (uint64_t)index * 8;
    emex64_memory_t *mem = ac97->machine->memory;

    if(!EMEX64_IN_PHYS_MEMORY(entry_addr, 8, mem->memory, mem->memory_size))
    {
        fprintf(stderr, "[ac97] fetch_bdle OOB: entry_addr=0x%llx bdl_base=0x%llx index=%u memsize=0x%llx\n",
                (unsigned long long)entry_addr,
                (unsigned long long)ac97->bm_bdl_base,
                index,
                (unsigned long long)mem->memory_size);
        return false;
    }

    const uint8_t *raw = mem->memory + entry_addr;
    ac97->bdl[index].base   = (uint32_t)raw[0]
                            | ((uint32_t)raw[1] << 8)
                            | ((uint32_t)raw[2] << 16)
                            | ((uint32_t)raw[3] << 24);
    ac97->bdl[index].length = (uint16_t)raw[4] | ((uint16_t)raw[5] << 8);
    ac97->bdl[index].flags  = (uint16_t)raw[6] | ((uint16_t)raw[7] << 8);

    static bool _dbg_bdle_once = false;
    if(!_dbg_bdle_once && index == 0)
    {
        _dbg_bdle_once = true;
        fprintf(stderr, "[ac97] bdle[0]: base=0x%08x length=%u flags=0x%04x\n",
                ac97->bdl[0].base, ac97->bdl[0].length, ac97->bdl[0].flags);
        fflush(stderr);
    }

    return true;
}

// ring head is written only by the VM and ring_tail only by the audio

static inline uint32_t ac97_ring_free(emex64_ac97_t *ac97)
{
    uint32_t head = atomic_load_explicit(&ac97->ring_head, memory_order_relaxed);
    uint32_t tail = atomic_load_explicit(&ac97->ring_tail, memory_order_acquire);
    return (AC97_RING_SIZE - 1) - ((head - tail) & AC97_RING_MASK);
}

static inline void ac97_ring_write(emex64_ac97_t *ac97, const uint8_t *src, uint32_t count)
{
    uint32_t head = atomic_load_explicit(&ac97->ring_head, memory_order_relaxed);
    uint32_t mask = AC97_RING_MASK;
    uint32_t till_wrap = AC97_RING_SIZE - (head & mask);
    if(count <= till_wrap)
    {
        memcpy(&ac97->ring[head & mask], src, count);
    }
    else
    {
        memcpy(&ac97->ring[head & mask], src, till_wrap);
        memcpy(&ac97->ring[0], src + till_wrap, count - till_wrap);
    }
    atomic_store_explicit(&ac97->ring_head, (head + count) & mask, memory_order_release);
}

static void ac97_dma_drain(emex64_ac97_t *ac97)
{
    emex64_memory_t *mem = ac97->machine->memory;

    for(;;)
    {
        if(ac97->bm_status & AC97_BM_STATUS_BCIS)
        {
            pthread_mutex_lock(&ac97->dma_mutex);
            while((ac97->bm_status & AC97_BM_STATUS_BCIS) &&
                  !atomic_load_explicit(&ac97->dma_stop, memory_order_acquire))
                pthread_cond_wait(&ac97->dma_cond, &ac97->dma_mutex);
            pthread_mutex_unlock(&ac97->dma_mutex);

            if(atomic_load_explicit(&ac97->dma_stop, memory_order_acquire))
                return;
        }

        if((ac97->bm_status & AC97_BM_STATUS_DCH) || !(ac97->bm_ctrl & AC97_BM_CTRL_RUN))
            return;

        if(ac97->samples_remaining == 0)
        {
            if(ac97->bm_civ == ((ac97->bm_lvi + 1) & 0x1F))
            {
                ac97->bm_status |= AC97_BM_STATUS_DCH | AC97_BM_STATUS_CELV;
                if((ac97->bm_ctrl & AC97_BM_CTRL_LVBIE) &&
                   !(ac97->bm_status & AC97_BM_STATUS_LVBCI))
                {
                    ac97->bm_status |= AC97_BM_STATUS_LVBCI;
                    emex64_raise_interrupt(ac97->machine, EMEX64_IRQ_AC97);
                }
                return;
            }

            ac97->bm_status &= ~AC97_BM_STATUS_CELV;

            if(!ac97_fetch_bdle(ac97, ac97->bm_civ))
            {
                ac97->bm_status |= AC97_BM_STATUS_DCH | AC97_BM_STATUS_FIFOE;
                if(ac97->bm_ctrl & AC97_BM_CTRL_FEIE)
                    emex64_raise_interrupt(ac97->machine, EMEX64_IRQ_AC97);
                return;
            }

            ac97->samples_remaining = (ac97->bdl[ac97->bm_civ].length == 0)
                                          ? 65536u
                                          : ac97->bdl[ac97->bm_civ].length;

            ac97->bm_piv  = (ac97->bm_civ + 1) & 0x1F;
            ac97->bm_picb = (uint16_t)(ac97->samples_remaining > 0xFFFF
                                           ? 0xFFFF
                                           : ac97->samples_remaining);
        }

        static const uint32_t bytes_per_sample = 4;

        uint32_t free_bytes       = ac97_ring_free(ac97);
        uint32_t can_push_samples = free_bytes / bytes_per_sample;

        if(can_push_samples == 0)
        {
            struct timespec ts = { .tv_sec = 0, .tv_nsec = 500000L };
            nanosleep(&ts, NULL);
            continue;
        }

        uint32_t todo = (can_push_samples < ac97->samples_remaining)
                            ? can_push_samples
                            : ac97->samples_remaining;

        uint64_t src_addr = (uint64_t)ac97->bdl[ac97->bm_civ].base
                          + ((uint64_t)(ac97->bdl[ac97->bm_civ].length - ac97->samples_remaining)
                             * bytes_per_sample);

        uint64_t copy_bytes = (uint64_t)todo * bytes_per_sample;

        if(EMEX64_IN_PHYS_MEMORY(src_addr, copy_bytes, mem->memory, mem->memory_size))
            ac97_ring_write(ac97, mem->memory + src_addr, (uint32_t)copy_bytes);
        else
        {
            static bool _dbg_oob_once = false;
            if(!_dbg_oob_once)
            {
                _dbg_oob_once = true;
                fprintf(stderr, "[ac97] src OOB: src_addr=0x%llx copy_bytes=%llu civ=%u\n",
                        (unsigned long long)src_addr, (unsigned long long)copy_bytes, ac97->bm_civ);
                fflush(stderr);
            }
        }

        ac97->samples_remaining -= todo;
        ac97->bm_picb = (uint16_t)(ac97->samples_remaining > 0xFFFF
                                       ? 0xFFFF
                                       : ac97->samples_remaining);

        if(ac97->samples_remaining == 0)
        {
            uint8_t  finished_idx = ac97->bm_civ;
            uint16_t flags        = ac97->bdl[finished_idx].flags;
            uint8_t  next_civ     = (finished_idx + 1) & 0x1F;
            bool     has_ioc      = (flags & AC97_BDL_FLAG_IOC) && (ac97->bm_ctrl & AC97_BM_CTRL_IOCE);

            if(finished_idx == ac97->bm_lvi)
            {
                ac97->bm_status |= AC97_BM_STATUS_CELV | AC97_BM_STATUS_DCH;
                if((ac97->bm_ctrl & AC97_BM_CTRL_LVBIE) &&
                   !(ac97->bm_status & AC97_BM_STATUS_LVBCI))
                {
                    ac97->bm_status |= AC97_BM_STATUS_LVBCI;
                    emex64_raise_interrupt(ac97->machine, EMEX64_IRQ_AC97);
                }
            }

            if(has_ioc)
            {
                ac97->bm_civ_next = next_civ;
                ac97->bm_status  |= AC97_BM_STATUS_BCIS;
                emex64_raise_interrupt(ac97->machine, EMEX64_IRQ_AC97);
            }
            else
            {
                ac97->bm_civ      = next_civ;
                ac97->bm_civ_next = next_civ;
            }
        }
    }
}

static void *ac97_dma_thread(void *arg)
{
    emex64_ac97_t *ac97 = (emex64_ac97_t *)arg;

    pthread_mutex_lock(&ac97->dma_mutex);

    while(!atomic_load_explicit(&ac97->dma_stop, memory_order_acquire))
    {
        pthread_cond_wait(&ac97->dma_cond, &ac97->dma_mutex);

        if(atomic_load_explicit(&ac97->dma_stop, memory_order_acquire))
            break;

        pthread_mutex_unlock(&ac97->dma_mutex);
        ac97_dma_drain(ac97);
        pthread_mutex_lock(&ac97->dma_mutex);
    }

    pthread_mutex_unlock(&ac97->dma_mutex);
    return NULL;
}
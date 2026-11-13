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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/board/sound.h>

#if defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>
#endif /* __APPLE__ */

#define SOUND_QUEUE_BUFFERS         3
#define SOUND_QUEUE_FRAMES          512
#define SOUND_PUMP_FRAMES           441

struct emex64_sound_out {
    emex64_sound_render_t render;
    void *context;

#if defined(__APPLE__)
    AudioQueueRef queue;
    AudioQueueBufferRef buffers[SOUND_QUEUE_BUFFERS];
#endif /* __APPLE__ */

    Boolean pumping;
    Boolean stop;
    pthread_t pump;
    FILE *wav;
    UInt32 wav_bytes;
};

#if defined(__APPLE__)

static void sound_queue_callback(void *context,
                                 AudioQueueRef queue,
                                 AudioQueueBufferRef buffer)
{
    emex64_sound_out_t *out = context;
    UInt32 frames = buffer->mAudioDataBytesCapacity / 4;
    out->render(out->context, (SInt16 *)buffer->mAudioData, frames);
    buffer->mAudioDataByteSize = frames * 4;
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

static Boolean sound_queue_open(emex64_sound_out_t *out)
{
    AudioStreamBasicDescription format;
    memset(&format, 0, sizeof(format));
    format.mSampleRate = EMEX64_SOUND_RATE;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mBitsPerChannel = 16;
    format.mChannelsPerFrame = 2;
    format.mBytesPerFrame = 4;
    format.mFramesPerPacket = 1;
    format.mBytesPerPacket = 4;

    if(AudioQueueNewOutput(&format, sound_queue_callback, out, NULL, NULL, 0, &out->queue) != noErr)
    {
        out->queue = NULL;
        return false;
    }

    for(UInt32 i = 0; i < SOUND_QUEUE_BUFFERS; i++)
    {
        if(AudioQueueAllocateBuffer(out->queue, SOUND_QUEUE_FRAMES * 4, &out->buffers[i]) != noErr)
        {
            AudioQueueDispose(out->queue, true);
            out->queue = NULL;
            return false;
        }
        sound_queue_callback(out, out->queue, out->buffers[i]);
    }

    if(AudioQueueStart(out->queue, NULL) != noErr)
    {
        AudioQueueDispose(out->queue, true);
        out->queue = NULL;
        return false;
    }
    return true;
}

#endif /* __APPLE__ */

static void sound_put32(UInt8 *p,
                        UInt32 v)
{
    p[0] = (UInt8)v;
    p[1] = (UInt8)(v >> 8);
    p[2] = (UInt8)(v >> 16);
    p[3] = (UInt8)(v >> 24);
}

static void sound_wav_header(emex64_sound_out_t *out)
{
    UInt8 h[44];
    memcpy(h, "RIFF", 4);
    sound_put32(h + 4, 36 + out->wav_bytes);
    memcpy(h + 8, "WAVEfmt ", 8);
    sound_put32(h + 16, 16);
    h[20] = 1; h[21] = 0;
    h[22] = 2; h[23] = 0;
    sound_put32(h + 24, EMEX64_SOUND_RATE);
    sound_put32(h + 28, EMEX64_SOUND_RATE * 4);
    h[32] = 4; h[33] = 0;
    h[34] = 16; h[35] = 0;
    memcpy(h + 36, "data", 4);
    sound_put32(h + 40, out->wav_bytes);
    fseek(out->wav, 0, SEEK_SET);
    fwrite(h, 1, sizeof(h), out->wav);
}

static void *sound_pump_thread(void *arg)
{
    emex64_sound_out_t *out = arg;
    SInt16 samples[SOUND_PUMP_FRAMES * 2];

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    while(!__atomic_load_n(&out->stop, __ATOMIC_ACQUIRE))
    {
        out->render(out->context, samples, SOUND_PUMP_FRAMES);
        if(out->wav != NULL)
        {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            for(UInt32 i = 0; i < SOUND_PUMP_FRAMES * 2; i++)
            {
                samples[i] = (SInt16)__builtin_bswap16((UInt16)samples[i]);
            }
#endif /* defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ */
            fwrite(samples, 4, SOUND_PUMP_FRAMES, out->wav);
            out->wav_bytes += SOUND_PUMP_FRAMES * 4;

            if(out->wav_bytes % (EMEX64_SOUND_RATE * 4) == 0)
            {
                sound_wav_header(out);
                fseek(out->wav, 0, SEEK_END);
                fflush(out->wav);
            }
        }

        next.tv_nsec += 10000000L;
        if(next.tv_nsec >= 1000000000L)
        {
            next.tv_nsec -= 1000000000L;
            next.tv_sec++;
        }
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long long wait = (long long)(next.tv_sec - now.tv_sec) * 1000000000LL + (next.tv_nsec - now.tv_nsec);
        if(wait > 0)
        {
            struct timespec ts = { (time_t)(wait / 1000000000LL), (long)(wait % 1000000000LL) };
            nanosleep(&ts, NULL);
        }
        else if(wait < -500000000LL)
        {
            next = now;
        }
    }

    return NULL;
}

emex64_sound_out_t *emex64_sound_out_open(emex64_sound_render_t render,
                                          void *context,
                                          const char *wavPath)
{
    emex64_sound_out_t *out = calloc(1, sizeof(emex64_sound_out_t));
    if(out == NULL)
    {
        return NULL;
    }
    out->render = render;
    out->context = context;

    if(wavPath != NULL)
    {
        out->wav = fopen(wavPath, "wb");
        if(out->wav != NULL)
        {
            sound_wav_header(out);
        }
    }

#if defined(__APPLE__)
    if(out->wav == NULL && sound_queue_open(out))
    {
        return out;
    }
#endif /* __APPLE__ */

    if(pthread_create(&out->pump, NULL, sound_pump_thread, out) != 0)
    {
        if(out->wav != NULL)
        {
            fclose(out->wav);
        }
        free(out);
        return NULL;
    }
    out->pumping = true;
    return out;
}

void emex64_sound_out_close(emex64_sound_out_t *out)
{
    if(out == NULL)
    {
        return;
    }

#if defined(__APPLE__)
    if(out->queue != NULL)
    {
        AudioQueueStop(out->queue, true);
        AudioQueueDispose(out->queue, true);
    }
#endif /* __APPLE__ */

    if(out->pumping)
    {
        __atomic_store_n(&out->stop, true, __ATOMIC_RELEASE);
        pthread_join(out->pump, NULL);
    }
    if(out->wav != NULL)
    {
        sound_wav_header(out);
        fclose(out->wav);
    }
    free(out);
}

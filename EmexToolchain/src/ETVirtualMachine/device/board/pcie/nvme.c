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

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif /* defined(__linux__) && !defined(_GNU_SOURCE) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#if defined(__linux__)
#include <linux/falloc.h>
#endif /* defined(__linux__) */
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETVirtualMachine/device/board/pcie/nvme.h>

#define NVME_IDENTIFY_SIZE  4096
#define NVME_HOLE_ALIGN     4096ULL

static void nvme_wr16(UInt8 *p,
                      UInt16 v)
{
    p[0] = (UInt8)v;
    p[1] = (UInt8)(v >> 8);
}

static void nvme_wr32(UInt8 *p,
                      UInt32 v)
{
    for(int i = 0; i < 4; i++)
    {
        p[i] = (UInt8)(v >> (8 * i));
    }
}

static void nvme_wr64(UInt8 *p,
                      UInt64 v)
{
    for(int i = 0; i < 8; i++)
    {
        p[i] = (UInt8)(v >> (8 * i));
    }
}

static UInt16 nvme_rd16(const UInt8 *p)
{
    return (UInt16)(p[0] | (p[1] << 8));
}

static UInt32 nvme_rd32(const UInt8 *p)
{
    UInt32 v = 0;
    for(int i = 0; i < 4; i++)
    {
        v |= (UInt32)p[i] << (8 * i);
    }
    return v;
}

static UInt64 nvme_rd64(const UInt8 *p)
{
    UInt64 v = 0;
    for(int i = 0; i < 8; i++)
    {
        v |= (UInt64)p[i] << (8 * i);
    }
    return v;
}

static void nvme_text(UInt8 *dst,
                      UInt64 size,
                      const char *text)
{
    UInt64 len = strlen(text);
    memset(dst, ' ', size);
    memcpy(dst, text, len < size ? len : size);
}

static void nvme_reset(emex64_nvme_t *nvme)
{
    nvme->cc = 0;
    nvme->csts = 0;
    nvme->aqa = 0;
    nvme->asq = 0;
    nvme->acq = 0;
    nvme->intms = 0;
    nvme->pageSize = 4096;
    nvme->aerCount = 0;
    memset(nvme->sq, 0, sizeof(nvme->sq));
    memset(nvme->cq, 0, sizeof(nvme->cq));
    memset(nvme->features, 0, sizeof(nvme->features));
}

static void nvme_flush(emex64_nvme_t *nvme)
{
    for(UInt8 n = 0; n < nvme->nsCount; n++)
    {
        if(!nvme->ns[n].readonly)
        {
            fsync(nvme->ns[n].fd);
        }
    }
}

static void nvme_enable(emex64_nvme_t *nvme)
{
    UInt32 sqSize = (UInt32)(nvme->aqa & 0xFFF) + 1;
    UInt32 cqSize = (UInt32)((nvme->aqa >> 16) & 0xFFF) + 1;
    UInt64 mps = NVME_CC_MPS(nvme->cc);

    if(mps > 4 || NVME_CC_IOSQES(nvme->cc) != 6 || NVME_CC_IOCQES(nvme->cc) != 4 ||
       nvme->asq == 0 || nvme->acq == 0 || sqSize < 2 || cqSize < 2 ||
       sqSize > NVME_MAX_QUEUE_ENTRIES || cqSize > NVME_MAX_QUEUE_ENTRIES)
    {
        nvme->csts = NVME_CSTS_CFS;
        return;
    }

    nvme->pageSize = 4096ULL << mps;
    memset(nvme->sq, 0, sizeof(nvme->sq));
    memset(nvme->cq, 0, sizeof(nvme->cq));
    nvme->aerCount = 0;
    nvme->sq[0].valid = true;
    nvme->sq[0].base = nvme->asq & ~(nvme->pageSize - 1);
    nvme->sq[0].size = sqSize;
    nvme->sq[0].cqid = 0;
    nvme->cq[0].valid = true;
    nvme->cq[0].irq = true;
    nvme->cq[0].base = nvme->acq & ~(nvme->pageSize - 1);
    nvme->cq[0].size = cqSize;
    nvme->csts = NVME_CSTS_RDY;
}

static void nvme_update_irq(emex64_nvme_t *nvme)
{
    if(nvme->intms & 1)
    {
        return;
    }
    for(UInt32 q = 0; q < NVME_MAX_QUEUES; q++)
    {
        if(nvme->cq[q].valid && nvme->cq[q].irq && nvme->cq[q].head != nvme->cq[q].tail)
        {
            emex64_pcie_raise_irq(&nvme->function);
            return;
        }
    }
}

typedef UInt16 (*nvme_piece_fn)(void *context, UInt8 *memory, UInt64 offset, UInt64 length);

static UInt16 nvme_walk(emex64_nvme_t *nvme,
                        UInt64 prp1,
                        UInt64 prp2,
                        UInt64 length,
                        Boolean toSystem,
                        nvme_piece_fn fn,
                        void *context)
{
    UInt64 page = nvme->pageSize;
    UInt64 done = 0;
    UInt64 list = 0;
    UInt64 index = 0;
    UInt64 perList = page / 8;

    while(done < length)
    {
        UInt64 address;
        if(done == 0)
        {
            address = prp1;
        }
        else if(list == 0 && length - done <= page && done <= page)
        {
            address = prp2;
            if(address & (page - 1))
            {
                return NVME_SC_INVALID_FIELD | NVME_SC_DNR;
            }
        }
        else
        {
            if(list == 0)
            {
                list = prp2;
                index = (list & (page - 1)) / 8;
                list &= ~(page - 1);
                if(prp2 & 7)
                {
                    return NVME_SC_INVALID_FIELD | NVME_SC_DNR;
                }
            }
            if(index == perList - 1 && length - done > page)
            {
                UInt8 *entry = emex64_pcie_dma_map(&nvme->function, list + index * 8, 8, false);
                if(entry == NULL)
                {
                    return NVME_SC_DATA_XFER_ERROR;
                }
                list = nvme_rd64(entry) & ~(page - 1);
                index = 0;
                if(list == 0)
                {
                    return NVME_SC_INVALID_FIELD | NVME_SC_DNR;
                }
            }
            UInt8 *entry = emex64_pcie_dma_map(&nvme->function, list + index * 8, 8, false);
            if(entry == NULL)
            {
                return NVME_SC_DATA_XFER_ERROR;
            }
            address = nvme_rd64(entry);
            index++;
            if(address & (page - 1))
            {
                return NVME_SC_INVALID_FIELD | NVME_SC_DNR;
            }
        }

        UInt64 piece = page - (address & (page - 1));
        if(piece > length - done)
        {
            piece = length - done;
        }
        UInt8 *memory = emex64_pcie_dma_map(&nvme->function, address, piece, toSystem);
        if(memory == NULL)
        {
            return NVME_SC_DATA_XFER_ERROR;
        }
        UInt16 status = fn(context, memory, done, piece);
        if(status != NVME_SC_SUCCESS)
        {
            return status;
        }
        done += piece;
    }
    return NVME_SC_SUCCESS;
}

static UInt16 nvme_piece_out(void *context,
                             UInt8 *memory,
                             UInt64 offset,
                             UInt64 length)
{
    memcpy(memory, (const UInt8 *)context + offset, length);
    return NVME_SC_SUCCESS;
}

static UInt16 nvme_piece_in(void *context,
                            UInt8 *memory,
                            UInt64 offset,
                            UInt64 length)
{
    memcpy((UInt8 *)context + offset, memory, length);
    return NVME_SC_SUCCESS;
}

static UInt16 nvme_reply(emex64_nvme_t *nvme,
                         const UInt8 *sqe,
                         const UInt8 *buffer,
                         UInt64 length)
{
    return nvme_walk(nvme, nvme_rd64(sqe + 24), nvme_rd64(sqe + 32), length, true, nvme_piece_out, (void *)buffer);
}

static UInt16 nvme_fetch(emex64_nvme_t *nvme,
                         const UInt8 *sqe,
                         UInt8 *buffer,
                         UInt64 length)
{
    return nvme_walk(nvme, nvme_rd64(sqe + 24), nvme_rd64(sqe + 32), length, false, nvme_piece_in, buffer);
}

static emex64_nvme_ns_t *nvme_namespace(emex64_nvme_t *nvme,
                                        UInt32 nsid)
{
    if(nsid == 0 || nsid > nvme->nsCount)
    {
        return NULL;
    }
    return &nvme->ns[nsid - 1];
}

static void nvme_identify_controller(emex64_nvme_t *nvme,
                                     UInt8 *id)
{
    char nqn[256];
    nvme_wr16(id + 0, PCIE_VENDOR_EMEXLABS);
    nvme_wr16(id + 2, PCIE_VENDOR_EMEXLABS);
    nvme_text(id + 4, 20, nvme->serial);
    nvme_text(id + 24, 40, "emex64 NAND");
    nvme_text(id + 64, 8, "1.0");
    id[77] = NVME_MDTS;
    nvme_wr16(id + 78, 1);
    nvme_wr32(id + 80, (UInt32)NVME_VERSION);
    id[111] = 1;
    id[258] = 3;
    id[259] = NVME_MAX_AERS - 1;
    id[260] = 0x03;
    nvme_wr16(id + 266, 343);
    nvme_wr16(id + 268, 358);
    id[512] = 0x66;
    id[513] = 0x44;
    nvme_wr32(id + 516, nvme->nsCount);
    nvme_wr16(id + 520, (1 << 2) | (1 << 3));
    id[525] = 1;
    snprintf(nqn, sizeof(nqn), "nqn.2026-11.org.emexlabs:emex64-nand:%.20s", nvme->serial);
    memcpy(id + 768, nqn, strlen(nqn));
    nvme_wr16(id + 2048, 300);
}

static void nvme_identify_namespace(emex64_nvme_ns_t *ns,
                                    UInt8 *id)
{
    nvme_wr64(id + 0, ns->blocks);
    nvme_wr64(id + 8, ns->blocks);
    nvme_wr64(id + 16, ns->blocks);
    id[99] = ns->readonly ? 1 : 0;
    memcpy(id + 104, ns->nguid, 16);
    nvme_wr32(id + 128, (UInt32)NVME_LBA_SHIFT << 16);
}

static UInt16 nvme_identify(emex64_nvme_t *nvme,
                            const UInt8 *sqe)
{
    static UInt8 id[NVME_IDENTIFY_SIZE];
    UInt32 nsid = nvme_rd32(sqe + 4);
    UInt8 cns = sqe[40];
    emex64_nvme_ns_t *ns = nvme_namespace(nvme, nsid);

    memset(id, 0, sizeof(id));
    switch(cns)
    {
        case 0x00:
            if(nsid == 0xFFFFFFFF || nsid == 0)
            {
                return NVME_SC_INVALID_NS | NVME_SC_DNR;
            }
            if(ns != NULL)
            {
                nvme_identify_namespace(ns, id);
            }
            break;
        case 0x01:
            nvme_identify_controller(nvme, id);
            break;
        case 0x02:
        case 0x07:
            for(UInt32 n = nsid >= 0xFFFFFFFE ? nvme->nsCount : nsid, k = 0; n < nvme->nsCount; n++, k++)
            {
                nvme_wr32(id + k * 4, n + 1);
            }
            break;
        case 0x03:
            if(ns == NULL)
            {
                return NVME_SC_INVALID_NS | NVME_SC_DNR;
            }
            id[0] = 2;
            id[1] = 16;
            memcpy(id + 4, ns->nguid, 16);
            break;
        case 0x05:
        case 0x06:
            break;
        case 0x08:
            if(ns == NULL)
            {
                return NVME_SC_INVALID_NS | NVME_SC_DNR;
            }
            id[14] = 1;
            break;
        default:
            return NVME_SC_INVALID_FIELD | NVME_SC_DNR;
    }

    return nvme_reply(nvme, sqe, id, sizeof(id));
}

static UInt16 nvme_get_log_page(emex64_nvme_t *nvme,
                                const UInt8 *sqe)
{
    static UInt8 log[4096];
    UInt32 cdw10 = nvme_rd32(sqe + 40);
    UInt32 cdw11 = nvme_rd32(sqe + 44);
    UInt64 offset = nvme_rd64(sqe + 48);
    UInt64 length = ((UInt64)(((cdw11 & 0xFFFF) << 16) | (cdw10 >> 16)) + 1) * 4;
    UInt64 size;

    memset(log, 0, sizeof(log));
    switch(cdw10 & 0xFF)
    {
        case 0x01:
            size = 64;
            break;
        case 0x02:
            size = 512;
            nvme_wr16(log + 1, 308);
            log[3] = 100;
            log[4] = 10;
            nvme_wr64(log + 32, nvme->unitsRead / 1000);
            nvme_wr64(log + 48, nvme->unitsWritten / 1000);
            nvme_wr64(log + 64, nvme->readCommands);
            nvme_wr64(log + 80, nvme->writeCommands);
            break;
        case 0x03:
            size = 512;
            log[0] = 1;
            nvme_text(log + 8, 8, "1.0");
            break;
        default:
            return NVME_SC_INVALID_LOG_PAGE | NVME_SC_DNR;
    }

    if(offset >= size)
    {
        return NVME_SC_INVALID_FIELD | NVME_SC_DNR;
    }
    if(length > size - offset)
    {
        length = size - offset;
    }

    return nvme_reply(nvme, sqe, log + offset, length);
}

static UInt16 nvme_features(emex64_nvme_t *nvme,
                            const UInt8 *sqe,
                            Boolean set,
                            UInt32 *result)
{
    UInt8 fid = sqe[40];
    UInt32 value = nvme_rd32(sqe + 44);

    switch(fid)
    {
        case 0x07:
            *result = (NVME_MAX_QUEUES - 2) | ((UInt32)(NVME_MAX_QUEUES - 2) << 16);
            return NVME_SC_SUCCESS;
        case 0x06:
            if(set)
            {
                return NVME_SC_SUCCESS;
            }
            *result = 1;
            return NVME_SC_SUCCESS;
        case 0x01:
        case 0x02:
        case 0x04:
        case 0x05:
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
            if(set)
            {
                nvme->features[fid] = value;
            }
            else
            {
                *result = nvme->features[fid];
            }
            return NVME_SC_SUCCESS;
        default:
            return NVME_SC_INVALID_FIELD | NVME_SC_DNR;
    }
}

static UInt16 nvme_create_cq(emex64_nvme_t *nvme,
                             const UInt8 *sqe)
{
    UInt32 cdw10 = nvme_rd32(sqe + 40);
    UInt32 cdw11 = nvme_rd32(sqe + 44);
    UInt16 qid = (UInt16)cdw10;
    UInt32 size = (cdw10 >> 16) + 1;

    if(qid == 0 || qid >= NVME_MAX_QUEUES || nvme->cq[qid].valid)
    {
        return NVME_SC_INVALID_QID | NVME_SC_DNR;
    }
    if(size < 2 || size > NVME_MAX_QUEUE_ENTRIES)
    {
        return NVME_SC_INVALID_QSIZE | NVME_SC_DNR;
    }

    if(!(cdw11 & 1) || (nvme_rd64(sqe + 24) & (nvme->pageSize - 1)))
    {
        return NVME_SC_INVALID_FIELD | NVME_SC_DNR;
    }

    memset(&nvme->cq[qid], 0, sizeof(nvme->cq[qid]));
    nvme->cq[qid].valid = true;
    nvme->cq[qid].irq = (cdw11 & 2) != 0;
    nvme->cq[qid].base = nvme_rd64(sqe + 24);
    nvme->cq[qid].size = size;
    return NVME_SC_SUCCESS;
}

static UInt16 nvme_create_sq(emex64_nvme_t *nvme,
                             const UInt8 *sqe)
{
    UInt32 cdw10 = nvme_rd32(sqe + 40);
    UInt32 cdw11 = nvme_rd32(sqe + 44);
    UInt16 qid = (UInt16)cdw10;
    UInt16 cqid = (UInt16)(cdw11 >> 16);
    UInt32 size = (cdw10 >> 16) + 1;

    if(qid == 0 || qid >= NVME_MAX_QUEUES || nvme->sq[qid].valid)
    {
        return NVME_SC_INVALID_QID | NVME_SC_DNR;
    }
    if(cqid == 0 || cqid >= NVME_MAX_QUEUES || !nvme->cq[cqid].valid)
    {
        return NVME_SC_INVALID_CQ | NVME_SC_DNR;
    }
    if(size < 2 || size > NVME_MAX_QUEUE_ENTRIES)
    {
        return NVME_SC_INVALID_QSIZE | NVME_SC_DNR;
    }
    if(!(cdw11 & 1) || (nvme_rd64(sqe + 24) & (nvme->pageSize - 1)))
    {
        return NVME_SC_INVALID_FIELD | NVME_SC_DNR;
    }

    memset(&nvme->sq[qid], 0, sizeof(nvme->sq[qid]));
    nvme->sq[qid].valid = true;
    nvme->sq[qid].base = nvme_rd64(sqe + 24);
    nvme->sq[qid].size = size;
    nvme->sq[qid].cqid = cqid;
    return NVME_SC_SUCCESS;
}

static UInt16 nvme_delete_queue(emex64_nvme_t *nvme,
                                const UInt8 *sqe,
                                Boolean completion)
{
    UInt16 qid = nvme_rd16(sqe + 40);

    if(qid == 0 || qid >= NVME_MAX_QUEUES)
    {
        return NVME_SC_INVALID_QID | NVME_SC_DNR;
    }
    if(!completion)
    {
        if(!nvme->sq[qid].valid)
        {
            return NVME_SC_INVALID_QID | NVME_SC_DNR;
        }
        nvme->sq[qid].valid = false;
        return NVME_SC_SUCCESS;
    }
    if(!nvme->cq[qid].valid)
    {
        return NVME_SC_INVALID_QID | NVME_SC_DNR;
    }

    for(UInt32 q = 1; q < NVME_MAX_QUEUES; q++)
    {
        if(nvme->sq[q].valid && nvme->sq[q].cqid == qid)
        {
            return NVME_SC_INVALID_QDEL | NVME_SC_DNR;
        }
    }

    nvme->cq[qid].valid = false;
    return NVME_SC_SUCCESS;
}

static Boolean nvme_admin(emex64_nvme_t *nvme,
                          const UInt8 *sqe,
                          UInt16 *status,
                          UInt32 *result)
{
    switch(sqe[0])
    {
        case NVME_ADM_DELETE_SQ:
            *status = nvme_delete_queue(nvme, sqe, false);
            break;
        case NVME_ADM_CREATE_SQ:
            *status = nvme_create_sq(nvme, sqe);
            break;
        case NVME_ADM_DELETE_CQ:
            *status = nvme_delete_queue(nvme, sqe, true);
            break;
        case NVME_ADM_CREATE_CQ:
            *status = nvme_create_cq(nvme, sqe);
            break;
        case NVME_ADM_GET_LOG_PAGE:
            *status = nvme_get_log_page(nvme, sqe);
            break;
        case NVME_ADM_IDENTIFY:
            *status = nvme_identify(nvme, sqe);
            break;
        case NVME_ADM_ABORT:
            *result = 1;
            *status = NVME_SC_SUCCESS;
            break;
        case NVME_ADM_SET_FEATURES:
            *status = nvme_features(nvme, sqe, true, result);
            break;
        case NVME_ADM_GET_FEATURES:
            *status = nvme_features(nvme, sqe, false, result);
            break;
        case NVME_ADM_ASYNC_EVENT:
            if(nvme->aerCount >= NVME_MAX_AERS)
            {
                *status = 0x105 | NVME_SC_DNR;
                break;
            }
            nvme->aerCid[nvme->aerCount++] = nvme_rd16(sqe + 2);
            return false;
        default:
            *status = NVME_SC_INVALID_OPCODE | NVME_SC_DNR;
            break;
    }
    return true;
}

typedef struct {
    emex64_nvme_ns_t *ns;
    off_t offset;
    Boolean write;
} nvme_transfer_t;

static UInt16 nvme_piece_disk(void *context,
                              UInt8 *memory,
                              UInt64 offset,
                              UInt64 length)
{
    nvme_transfer_t *transfer = context;
    UInt64 done = 0;

    while(done < length)
    {
        off_t at = transfer->offset + (off_t)(offset + done);
        ssize_t n = transfer->write ? pwrite(transfer->ns->fd, memory + done, length - done, at) : pread(transfer->ns->fd, memory + done, length - done, at);
        if(n <= 0)
        {
            return NVME_SC_INTERNAL;
        }
        done += (UInt64)n;
    }

    return NVME_SC_SUCCESS;
}

static Boolean nvme_punch(emex64_nvme_ns_t *ns,
                          UInt64 offset,
                          UInt64 length)
{
    UInt64 start = (offset + NVME_HOLE_ALIGN - 1) & ~(NVME_HOLE_ALIGN - 1);
    UInt64 end = (offset + length) & ~(NVME_HOLE_ALIGN - 1);

    if(end <= start)
    {
        return false;
    }
#if defined(__APPLE__) && defined(F_PUNCHHOLE)
    struct fpunchhole hole = { .fp_offset = (off_t)start, .fp_length = (off_t)(end - start) };
    return fcntl(ns->fd, F_PUNCHHOLE, &hole) == 0;
#elif defined(__linux__) && defined(FALLOC_FL_PUNCH_HOLE)
    return fallocate(ns->fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, (off_t)start, (off_t)(end - start)) == 0;
#else
    return false;
#endif /* defined(__APPLE__) && defined(F_PUNCHHOLE) || defined(__linux__) && defined(FALLOC_FL_PUNCH_HOLE) */
}

static UInt16 nvme_zeroes(emex64_nvme_ns_t *ns,
                          UInt64 offset,
                          UInt64 length)
{
    static const UInt8 zero[65536];
    UInt64 start = (offset + NVME_HOLE_ALIGN - 1) & ~(NVME_HOLE_ALIGN - 1);
    UInt64 end = (offset + length) & ~(NVME_HOLE_ALIGN - 1);
    UInt64 done = 0;

    Boolean hole = nvme_punch(ns, offset, length);
    while(done < length)
    {
        UInt64 at = offset + done;
        UInt64 n = length - done;
        if(hole && at >= start && at < end)
        {
            done = end - offset;
            continue;
        }
        if(hole && at < start && n > start - at)
        {
            n = start - at;
        }
        if(n > sizeof(zero))
        {
            n = sizeof(zero);
        }
        if(pwrite(ns->fd, zero, n, (off_t)at) != (ssize_t)n)
        {
            return NVME_SC_INTERNAL;
        }
        done += n;
    }

    return NVME_SC_SUCCESS;
}

static UInt16 nvme_io(emex64_nvme_t *nvme,
                      const UInt8 *sqe)
{
    UInt8 opcode = sqe[0];
    emex64_nvme_ns_t *ns = nvme_namespace(nvme, nvme_rd32(sqe + 4));
    UInt64 lba = nvme_rd64(sqe + 40);
    UInt64 count = (UInt64)nvme_rd16(sqe + 48) + 1;

    if(ns == NULL)
    {
        if(opcode == NVME_CMD_FLUSH && nvme_rd32(sqe + 4) == 0xFFFFFFFF)
        {
            nvme_flush(nvme);
            return NVME_SC_SUCCESS;
        }
        return NVME_SC_INVALID_NS | NVME_SC_DNR;
    }

    switch(opcode)
    {
        case NVME_CMD_FLUSH:
            if(!ns->readonly && fsync(ns->fd) != 0)
            {
                return NVME_SC_INTERNAL;
            }
            return NVME_SC_SUCCESS;
        case NVME_CMD_READ:
        case NVME_CMD_WRITE:
        {
            nvme_transfer_t transfer = { .ns = ns, .offset = (off_t)(lba << NVME_LBA_SHIFT), .write = opcode == NVME_CMD_WRITE };
            if(lba >= ns->blocks || count > ns->blocks - lba)
            {
                return NVME_SC_LBA_RANGE | NVME_SC_DNR;
            }
            if((count << NVME_LBA_SHIFT) > (nvme->pageSize << NVME_MDTS))
            {
                return NVME_SC_INVALID_FIELD | NVME_SC_DNR;
            }
            if(transfer.write && ns->readonly)
            {
                return NVME_SC_NS_WRITE_PROTECTED | NVME_SC_DNR;
            }
            if(transfer.write)
            {
                __atomic_fetch_add(&nvme->writeCommands, 1, __ATOMIC_RELAXED);
                __atomic_fetch_add(&nvme->unitsWritten, count, __ATOMIC_RELAXED);
            }
            else
            {
                __atomic_fetch_add(&nvme->readCommands, 1, __ATOMIC_RELAXED);
                __atomic_fetch_add(&nvme->unitsRead, count, __ATOMIC_RELAXED);
            }
            return nvme_walk(nvme, nvme_rd64(sqe + 24), nvme_rd64(sqe + 32), count << NVME_LBA_SHIFT, !transfer.write, nvme_piece_disk, &transfer);
        }
        case NVME_CMD_WRITE_ZEROES:
            if(lba >= ns->blocks || count > ns->blocks - lba)
            {
                return NVME_SC_LBA_RANGE | NVME_SC_DNR;
            }
            if(ns->readonly)
            {
                return NVME_SC_NS_WRITE_PROTECTED | NVME_SC_DNR;
            }
            return nvme_zeroes(ns, lba << NVME_LBA_SHIFT, count << NVME_LBA_SHIFT);
        case NVME_CMD_DSM:
        {
            UInt8 ranges[4096];
            UInt32 n = (UInt32)sqe[40] + 1;
            UInt16 status = nvme_fetch(nvme, sqe, ranges, (UInt64)n * 16);
            if(status != NVME_SC_SUCCESS)
            {
                return status;
            }
            if(!(nvme_rd32(sqe + 44) & (1 << 2)) || ns->readonly)
            {
                return NVME_SC_SUCCESS;
            }
            for(UInt32 r = 0; r < n; r++)
            {
                UInt64 blocks = nvme_rd32(ranges + r * 16 + 4);
                UInt64 first = nvme_rd64(ranges + r * 16 + 8);
                if(first >= ns->blocks || blocks > ns->blocks - first)
                {
                    return NVME_SC_LBA_RANGE | NVME_SC_DNR;
                }
                nvme_punch(ns, first << NVME_LBA_SHIFT, blocks << NVME_LBA_SHIFT);
            }
            return NVME_SC_SUCCESS;
        }
        default:
            return NVME_SC_INVALID_OPCODE | NVME_SC_DNR;
    }
}

static void nvme_process(emex64_nvme_t *nvme,
                         UInt16 qid)
{
    emex64_nvme_sq_t *sq = &nvme->sq[qid];
    emex64_nvme_cq_t *cq;

    if(!(nvme->csts & NVME_CSTS_RDY) || !sq->valid)
    {
        return;
    }
    cq = &nvme->cq[sq->cqid];

    while(sq->head != sq->tail)
    {
        if(!cq->valid || (cq->tail + 1) % cq->size == cq->head)
        {
            break;
        }

        UInt8 *sqe = emex64_pcie_dma_map(&nvme->function, sq->base + (UInt64)sq->head * NVME_SQE_SIZE, NVME_SQE_SIZE, false);
        UInt8 *cqe = emex64_pcie_dma_map(&nvme->function, cq->base + (UInt64)cq->tail * NVME_CQE_SIZE, NVME_CQE_SIZE, true);
        if(sqe == NULL || cqe == NULL)
        {
            nvme->csts |= NVME_CSTS_CFS;
            return;
        }
        sq->head = (sq->head + 1) % sq->size;

        UInt16 status = NVME_SC_SUCCESS;
        UInt32 result = 0;
        if(qid == 0)
        {
            if(!nvme_admin(nvme, sqe, &status, &result))
            {
                continue;
            }
        }
        else
        {
            status = nvme_io(nvme, sqe);
        }

        nvme_wr32(cqe + 0, result);
        nvme_wr32(cqe + 4, 0);
        nvme_wr16(cqe + 8, (UInt16)sq->head);
        nvme_wr16(cqe + 10, qid);
        nvme_wr16(cqe + 12, nvme_rd16(sqe + 2));
        nvme_wr16(cqe + 14, (UInt16)((status << 1) | (cq->phase ? 0 : 1)));
        cq->tail = (cq->tail + 1) % cq->size;
        if(cq->tail == 0)
        {
            cq->phase = !cq->phase;
        }
    }

    nvme_update_irq(nvme);
}

static UInt64 nvme_bar_read(emex64_pcie_function_t *function,
                            UInt8 bar,
                            UInt64 offset)
{
    emex64_nvme_t *nvme = function->context;

    switch(offset)
    {
        case NVME_REG_CAP:
            return (NVME_MAX_QUEUE_ENTRIES - 1) | (1ULL << 16) | (1ULL << 24) | (1ULL << 37) | (4ULL << 52);
        case NVME_REG_VS:
            return NVME_VERSION;
        case NVME_REG_INTMS:
        case NVME_REG_INTMC:
            return nvme->intms;
        case NVME_REG_CC:
            return nvme->cc;
        case NVME_REG_CSTS:
            return nvme->csts;
        case NVME_REG_AQA:
            return nvme->aqa;
        case NVME_REG_ASQ:
            return nvme->asq;
        case NVME_REG_ACQ:
            return nvme->acq;
        default:
            return 0;
    }
}

static void nvme_bar_write(emex64_pcie_function_t *function,
                           UInt8 bar,
                           UInt64 offset,
                           UInt64 value)
{
    emex64_nvme_t *nvme = function->context;

    if(offset >= NVME_REG_SQ_TAIL(0) && offset < NVME_REG_SQ_TAIL(NVME_MAX_QUEUES))
    {
        UInt16 qid = (UInt16)((offset - NVME_REG_SQ_TAIL(0)) / 0x10);
        if(!(nvme->csts & NVME_CSTS_RDY))
        {
            return;
        }
        if((offset & 0xF) == 0)
        {
            if(nvme->sq[qid].valid && value < nvme->sq[qid].size)
            {
                nvme->sq[qid].tail = (UInt32)value;
                nvme_process(nvme, qid);
            }
            return;
        }
        if(nvme->cq[qid].valid && value < nvme->cq[qid].size)
        {
            nvme->cq[qid].head = (UInt32)value;
            for(UInt16 q = 0; q < NVME_MAX_QUEUES; q++)
            {
                if(nvme->sq[q].valid && nvme->sq[q].cqid == qid && nvme->sq[q].head != nvme->sq[q].tail)
                {
                    nvme_process(nvme, q);
                }
            }
        }
        return;
    }

    switch(offset)
    {
        case NVME_REG_INTMS:
            nvme->intms |= value;
            break;
        case NVME_REG_INTMC:
            nvme->intms &= ~value;
            nvme_update_irq(nvme);
            break;
        case NVME_REG_CC:
        {
            Boolean was = (nvme->cc & NVME_CC_EN) != 0;
            Boolean now = (value & NVME_CC_EN) != 0;
            nvme->cc = value & 0xFFFFFFFFULL;
            if(!was && now)
            {
                nvme_enable(nvme);
            }
            else if(was && !now)
            {
                UInt64 aqa = nvme->aqa, asq = nvme->asq, acq = nvme->acq;
                nvme_flush(nvme);
                nvme_reset(nvme);
                nvme->aqa = aqa;
                nvme->asq = asq;
                nvme->acq = acq;
            }
            if(NVME_CC_SHN(value) != 0)
            {
                nvme_flush(nvme);
                nvme->csts = (nvme->csts & ~(3ULL << 2)) | NVME_CSTS_SHST_DONE;
            }
            else
            {
                nvme->csts &= ~(3ULL << 2);
            }
            break;
        }
        case NVME_REG_AQA:
            nvme->aqa = value & 0x0FFF0FFFULL;
            break;
        case NVME_REG_ASQ:
            nvme->asq = value;
            break;
        case NVME_REG_ACQ:
            nvme->acq = value;
            break;
        default:
            break;
    }
}

static const emex64_pcie_ops_t nvme_ops = {
    .bar_read = nvme_bar_read,
    .bar_write = nvme_bar_write,
};

emex64_nvme_t *emex64_nvme_alloc(emex64_pcie_t *pcie,
                                 UInt8 dev)
{
    emex64_nvme_t *nvme = calloc(1, sizeof(emex64_nvme_t));
    if(nvme == NULL)
    {
        return NULL;
    }

    for(UInt8 n = 0; n < NVME_MAX_NAMESPACES; n++)
    {
        nvme->ns[n].fd = -1;
    }
    snprintf(nvme->serial, sizeof(nvme->serial), "E64NAND0000000000");

    nvme->function.vendor = PCIE_VENDOR_EMEXLABS;
    nvme->function.device = NVME_PCI_DEVICE_ID;
    nvme->function.class_code = NVME_PCI_CLASS;
    nvme->function.revision = 1;
    nvme->function.bar_size[0] = NVME_BAR0_SIZE;
    nvme->function.ops = &nvme_ops;
    nvme->function.context = nvme;

    if(!emex64_pcie_attach(pcie, &nvme->function, dev, 0))
    {
        free(nvme);
        return NULL;
    }

    nvme_reset(nvme);
    return nvme;
}

Boolean emex64_nvme_attach_nand(emex64_nvme_t *nvme,
                                const char *path,
                                Boolean readonly)
{
    if(nvme == NULL || path == NULL || nvme->nsCount >= NVME_MAX_NAMESPACES)
    {
        return false;
    }

    emex64_nvme_ns_t *ns = &nvme->ns[nvme->nsCount];
    int fd = open(path, readonly ? O_RDONLY : O_RDWR);
    if(fd < 0)
    {
        diag_error(NULL, "nvme: cannot open NAND image '%s'", path);
        return false;
    }

    struct stat st;
    if(fstat(fd, &st) != 0 || st.st_size < NVME_LBA_SIZE)
    {
        diag_error(NULL, "nvme: NAND image '%s' is smaller than one block", path);
        close(fd);
        return false;
    }

    ns->fd = fd;
    ns->present = true;
    ns->readonly = readonly;
    ns->blocks = (UInt64)st.st_size >> NVME_LBA_SHIFT;

    UInt64 hash = 0xCBF29CE484222325ULL;
    for(const char *c = path; *c; c++)
    {
        hash = (hash ^ (UInt8)*c) * 0x100000001B3ULL;
    }
    nvme_wr64(ns->nguid, hash);
    nvme_wr64(ns->nguid + 8, 0xE64000000000ULL | (UInt64)(nvme->nsCount + 1));
    if(nvme->nsCount == 0)
    {
        snprintf(nvme->serial, sizeof(nvme->serial), "E64NAND%012llX", (unsigned long long)(hash & 0xFFFFFFFFFFFFULL));
    }
    nvme->nsCount++;
    return true;
}

void emex64_nvme_reset(emex64_nvme_t *nvme)
{
    if(nvme != NULL)
    {
        nvme_flush(nvme);
        nvme_reset(nvme);
    }
}

void emex64_nvme_dealloc(emex64_nvme_t *nvme)
{
    if(nvme == NULL)
    {
        return;
    }

    for(UInt8 n = 0; n < NVME_MAX_NAMESPACES; n++)
    {
        if(nvme->ns[n].fd >= 0)
        {
            close(nvme->ns[n].fd);
        }
    }
    free(nvme);
}

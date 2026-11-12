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
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/VM/device/board/pcie/sata.h>

static void sata_wr64(UInt8 *p,
                      UInt64 v)
{
    for(int i = 0; i < 8; i++)
    {
        p[i] = (UInt8)(v >> (8 * i));
    }
}

static UInt64 sata_rd64(const UInt8 *p)
{
    UInt64 v = 0;
    for(int i = 0; i < 8; i++)
    {
        v |= (UInt64)p[i] << (8 * i);
    }
    return v;
}

static void sata_port_reset(emex64_sata_port_t *port)
{
    port->clb = 0;
    port->cmd = 0;
    port->ci = 0;
    port->is = 0;
    port->ie = 0;
    port->serr = 0;
}

static void sata_reset(emex64_sata_t *sata)
{
    sata->ghc = 0;
    for(UInt8 p = 0; p < SATA_MAX_PORTS; p++)
    {
        sata_port_reset(&sata->ports[p]);
    }
}

static void sata_update_irq(emex64_sata_t *sata)
{
    for(UInt8 p = 0; p < SATA_MAX_PORTS; p++)
    {
        if(sata->ports[p].is & sata->ports[p].ie)
        {
            if(sata->ghc & SATA_GHC_IE)
            {
                emex64_pcie_raise_irq(&sata->function);
            }
            return;
        }
    }
}

static void sata_identify(emex64_sata_port_t *port,
                          UInt8 *buf)
{
    memset(buf, 0, SATA_SECTOR_SIZE);
    memcpy(buf + 0x00, port->model, sizeof(port->model));
    memcpy(buf + 0x28, port->serial, sizeof(port->serial));
    sata_wr64(buf + 0x40, port->sectors);
    sata_wr64(buf + 0x48, SATA_SECTOR_SIZE);
    sata_wr64(buf + 0x50, port->readonly ? 0 : 1);
    sata_wr64(buf + 0x58, SATA_MAX_SECTORS_PER_CMD);
}

static UInt8 sata_execute(emex64_sata_t *sata,
                          emex64_sata_port_t *port,
                          UInt8 *slot,
                          UInt64 *bytes)
{
    UInt8 opcode = (UInt8)sata_rd64(slot + SATA_SLOT_OPCODE);
    UInt64 lba = sata_rd64(slot + SATA_SLOT_LBA);
    UInt64 count = sata_rd64(slot + SATA_SLOT_COUNT);
    UInt64 buffer = sata_rd64(slot + SATA_SLOT_BUFFER);

    *bytes = 0;
    if(!port->present)
    {
        return SATA_ERR_NO_DEVICE;
    }

    switch(opcode)
    {
        case SATA_OP_IDENTIFY:
        {
            UInt8 *dst = emex64_pcie_dma_map(&sata->function, buffer, SATA_SECTOR_SIZE, true);
            if(dst == NULL)
            {
                return SATA_ERR_DMA;
            }
            sata_identify(port, dst);
            *bytes = SATA_SECTOR_SIZE;
            return SATA_ERR_NONE;
        }
        case SATA_OP_READ_DMA_EXT:
        case SATA_OP_WRITE_DMA_EXT:
        {
            Boolean write = opcode == SATA_OP_WRITE_DMA_EXT;
            if(count == 0 || count > SATA_MAX_SECTORS_PER_CMD || lba >= port->sectors || count > port->sectors - lba)
            {
                return SATA_ERR_OUT_OF_RANGE;
            }
            if(write && port->readonly)
            {
                return SATA_ERR_READ_ONLY;
            }

            UInt64 len = count * SATA_SECTOR_SIZE;
            UInt8 *mem = emex64_pcie_dma_map(&sata->function, buffer, len, !write);
            if(mem == NULL)
            {
                return SATA_ERR_DMA;
            }

            off_t off = (off_t)(lba * SATA_SECTOR_SIZE);
            UInt64 done = 0;
            while(done < len)
            {
                ssize_t n = write ? pwrite(port->fd, mem + done, len - done, off + (off_t)done)
                                  : pread(port->fd, mem + done, len - done, off + (off_t)done);
                if(n <= 0)
                {
                    *bytes = done;
                    return SATA_ERR_IO;
                }
                done += (UInt64)n;
            }
            *bytes = done;
            return SATA_ERR_NONE;
        }
        case SATA_OP_FLUSH_CACHE_EXT:
            if(!port->readonly && fsync(port->fd) != 0)
            {
                return SATA_ERR_IO;
            }
            return SATA_ERR_NONE;
        default:
            return SATA_ERR_BAD_COMMAND;
    }
}

static void sata_port_process(emex64_sata_t *sata,
                              emex64_sata_port_t *port)
{
    if(!(port->cmd & SATA_CMD_START))
    {
        return;
    }

    while(port->ci != 0)
    {
        UInt8 s = (UInt8)__builtin_ctzll(port->ci);
        UInt8 *slot = emex64_pcie_dma_map(&sata->function, port->clb + (UInt64)s * SATA_SLOT_SIZE, SATA_SLOT_SIZE, true);

        port->ci &= ~(1ULL << s);
        if(slot == NULL)
        {
            port->serr = SATA_ERR_DMA;
            port->is |= SATA_IS_ERROR;
            continue;
        }

        UInt64 bytes = 0;
        UInt8 err = sata_execute(sata, port, slot, &bytes);
        sata_wr64(slot + SATA_SLOT_BYTES, bytes);
        sata_wr64(slot + SATA_SLOT_STATUS, SATA_STATUS_DONE | (err ? SATA_STATUS_ERROR : 0) | ((UInt64)err << 8));
        if(err)
        {
            port->serr = err;
            port->is |= SATA_IS_ERROR;
        }
        port->is |= SATA_IS_DONE;
    }

    sata_update_irq(sata);
}

static UInt64 sata_bar_read(emex64_pcie_function_t *function,
                            UInt8 bar,
                            UInt64 offset)
{
    emex64_sata_t *sata = function->context;

    switch(offset)
    {
        case SATA_REG_CAP:
            return SATA_MAX_PORTS | (SATA_CMD_SLOTS << 8) | (1ULL << 32);
        case SATA_REG_GHC:
            return sata->ghc;
        case SATA_REG_IS:
        {
            UInt64 is = 0;
            for(UInt8 p = 0; p < SATA_MAX_PORTS; p++)
            {
                if(sata->ports[p].is)
                {
                    is |= 1ULL << p;
                }
            }
            return is;
        }
        case SATA_REG_PI:
            return (1ULL << SATA_MAX_PORTS) - 1;
        default:
            break;
    }

    if(offset < SATA_PORT_BASE(0) || offset >= SATA_PORT_BASE(SATA_MAX_PORTS))
    {
        return 0;
    }

    emex64_sata_port_t *port = &sata->ports[(offset - SATA_PORT_BASE(0)) / 0x100];
    switch(offset & 0xFF)
    {
        case SATA_PxSSTS:
            return port->present ? (SATA_SSTS_PRESENT | (port->readonly ? SATA_SSTS_READONLY : 0) | ((UInt64)SATA_DEV_ATA << 8)) : 0;
        case SATA_PxSECTORS:
            return port->present ? port->sectors : 0;
        case SATA_PxSECSZ:
            return SATA_SECTOR_SIZE;
        case SATA_PxCLB:
            return port->clb;
        case SATA_PxCMD:
            return port->cmd;
        case SATA_PxCI:
            return port->ci;
        case SATA_PxIS:
            return port->is;
        case SATA_PxIE:
            return port->ie;
        case SATA_PxSERR:
            return port->serr;
        default:
            return 0;
    }
}

static void sata_bar_write(emex64_pcie_function_t *function,
                           UInt8 bar,
                           UInt64 offset,
                           UInt64 value)
{
    emex64_sata_t *sata = function->context;

    if(offset == SATA_REG_GHC)
    {
        if(value & SATA_GHC_RESET)
        {
            sata_reset(sata);
            return;
        }
        sata->ghc = value & SATA_GHC_IE;
        sata_update_irq(sata);
        return;
    }

    if(offset == SATA_REG_IS)
    {
        for(UInt8 p = 0; p < SATA_MAX_PORTS; p++)
        {
            if(value & (1ULL << p))
            {
                sata->ports[p].is = 0;
            }
        }
        return;
    }

    if(offset < SATA_PORT_BASE(0) || offset >= SATA_PORT_BASE(SATA_MAX_PORTS))
    {
        return;
    }

    emex64_sata_port_t *port = &sata->ports[(offset - SATA_PORT_BASE(0)) / 0x100];
    switch(offset & 0xFF)
    {
        case SATA_PxCLB:
            port->clb = value & ~(UInt64)(SATA_SLOT_SIZE - 1);
            break;
        case SATA_PxCMD:
            port->cmd = value & SATA_CMD_START;
            sata_port_process(sata, port);
            break;
        case SATA_PxCI:
            port->ci |= value & 0xFFFFFFFFULL;
            sata_port_process(sata, port);
            break;
        case SATA_PxIS:
            port->is &= ~value;
            break;
        case SATA_PxIE:
            port->ie = value & (SATA_IS_DONE | SATA_IS_ERROR);
            sata_update_irq(sata);
            break;
        case SATA_PxSERR:
            port->serr &= ~value;
            break;
        default:
            break;
    }
}

static const emex64_pcie_ops_t sata_ops = {
    .bar_read = sata_bar_read,
    .bar_write = sata_bar_write,
};

emex64_sata_t *emex64_sata_alloc(emex64_pcie_t *pcie,
                                 UInt8 dev)
{
    emex64_sata_t *sata = calloc(1, sizeof(emex64_sata_t));
    if(sata == NULL)
    {
        return NULL;
    }

    for(UInt8 p = 0; p < SATA_MAX_PORTS; p++)
    {
        sata->ports[p].fd = -1;
    }

    sata->function.vendor = PCIE_VENDOR_EMEXLABS;
    sata->function.device = SATA_PCI_DEVICE_ID;
    sata->function.class_code = SATA_PCI_CLASS;
    sata->function.revision = 1;
    sata->function.bar_size[0] = SATA_BAR0_SIZE;
    sata->function.ops = &sata_ops;
    sata->function.context = sata;

    if(!emex64_pcie_attach(pcie, &sata->function, dev, 0))
    {
        free(sata);
        return NULL;
    }

    sata_reset(sata);
    return sata;
}

Boolean emex64_sata_attach_disk(emex64_sata_t *sata,
                                UInt8 portIndex,
                                const char *path,
                                Boolean readonly)
{
    if(sata == NULL || portIndex >= SATA_MAX_PORTS || path == NULL || sata->ports[portIndex].present)
    {
        return false;
    }

    emex64_sata_port_t *port = &sata->ports[portIndex];
    int fd = open(path, readonly ? O_RDONLY : O_RDWR);
    if(fd < 0)
    {
        diag_error(NULL, "sata: cannot open disk image '%s'", path);
        return false;
    }

    struct stat st;
    if(fstat(fd, &st) != 0 || st.st_size < SATA_SECTOR_SIZE)
    {
        diag_error(NULL, "sata: disk image '%s' is smaller than one sector", path);
        close(fd);
        return false;
    }

    port->fd = fd;
    port->present = true;
    port->readonly = readonly;
    port->sectors = (UInt64)st.st_size / SATA_SECTOR_SIZE;

    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    memset(port->model, ' ', sizeof(port->model));
    memset(port->serial, ' ', sizeof(port->serial));
    char model[64];
    snprintf(model, sizeof(model), "emex64 SATA disk %s", base);
    memcpy(port->model, model, strlen(model) < sizeof(port->model) ? strlen(model) : sizeof(port->model));
    char serial[24];
    snprintf(serial, sizeof(serial), "E64SATA%02u%08llX", portIndex, (unsigned long long)port->sectors);
    memcpy(port->serial, serial, strlen(serial) < sizeof(port->serial) ? strlen(serial) : sizeof(port->serial));
    return true;
}

void emex64_sata_reset(emex64_sata_t *sata)
{
    if(sata != NULL)
    {
        sata_reset(sata);
    }
}

void emex64_sata_dealloc(emex64_sata_t *sata)
{
    if(sata == NULL)
    {
        return;
    }

    for(UInt8 p = 0; p < SATA_MAX_PORTS; p++)
    {
        if(sata->ports[p].fd >= 0)
        {
            close(sata->ports[p].fd);
        }
    }
    free(sata);
}

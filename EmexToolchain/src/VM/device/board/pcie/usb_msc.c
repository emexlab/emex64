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
#include <EmexToolchain/VM/device/board/pcie/usb.h>

#define MSC_BLOCK_SIZE          512
#define MSC_EP_IN               1
#define MSC_EP_OUT              2

#define CBW_SIGNATURE           0x43425355u
#define CSW_SIGNATURE           0x53425355u
#define CBW_SIZE                31
#define CSW_SIZE                13

#define CSW_PASSED              0
#define CSW_FAILED              1
#define CSW_PHASE_ERROR         2

#define MSC_REQ_RESET           0xFF
#define MSC_REQ_GET_MAX_LUN     0xFE

#define SK_NO_SENSE             0x0
#define SK_MEDIUM_ERROR         0x3
#define SK_ILLEGAL_REQUEST      0x5
#define SK_DATA_PROTECT         0x7

enum msc_state {
    MSC_CBW,
    MSC_DATA_IN,
    MSC_DATA_OUT,
    MSC_CSW
};

enum msc_source {
    SRC_BUFFER,
    SRC_DISK,
    SRC_DISCARD
};

typedef struct {
    emex64_usb_device_t usb;
    int fd;
    Boolean readonly;
    UInt64 blocks;
    char serial[17];
    UInt8 device_descriptor[18];
    UInt8 config_descriptor[32];

    enum msc_state state;
    Boolean needs_reset;

    UInt32 tag;
    UInt32 host_length;
    UInt32 transfer;
    UInt32 process;
    UInt32 done;
    Boolean zlp;
    UInt8 status;
    enum msc_source source;
    UInt64 disk_offset;
    UInt8 buffer[64];

    UInt8 sense_key, asc, ascq;
} msc_t;

static UInt32 be32(const UInt8 *p)
{
    return ((UInt32)p[0] << 24) | ((UInt32)p[1] << 16) | ((UInt32)p[2] << 8) | p[3];
}

static UInt16 be16(const UInt8 *p)
{
    return (UInt16)((p[0] << 8) | p[1]);
}

static UInt64 be64(const UInt8 *p)
{
    return ((UInt64)be32(p) << 32) | be32(p + 4);
}

static void put_be32(UInt8 *p,
                     UInt32 v)
{
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = (UInt8)v;
}

static void put_be64(UInt8 *p,
                     UInt64 v)
{
    put_be32(p, (UInt32)(v >> 32)); put_be32(p + 4, (UInt32)v);
}

static UInt32 le32(const UInt8 *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((UInt32)p[3] << 24);
}

static void put_le32(UInt8 *p,
                     UInt32 v)
{
    p[0] = (UInt8)v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24;
}

static void msc_sense(msc_t *m,
                      UInt8 key,
                      UInt8 asc,
                      UInt8 ascq)
{
    m->sense_key = key;
    m->asc = asc;
    m->ascq = ascq;
}

typedef struct {
    enum { DIR_NONE, DIR_IN, DIR_OUT } dir;
    UInt32 length;
    Boolean ok;
} scsi_result_t;

static scsi_result_t scsi_fail(msc_t *m,
                               UInt8 key,
                               UInt8 asc,
                               UInt8 ascq)
{
    msc_sense(m, key, asc, ascq);
    return (scsi_result_t){ DIR_NONE, 0, false };
}

static scsi_result_t scsi_data_in(msc_t *m,
                                  UInt32 size,
                                  UInt32 alloc)
{
    m->source = SRC_BUFFER;
    msc_sense(m, SK_NO_SENSE, 0, 0);
    UInt32 n = size < alloc ? size : alloc;
    return (scsi_result_t){ n ? DIR_IN : DIR_NONE, n, true };
}

static scsi_result_t scsi_read_write(msc_t *m,
                                     Boolean write,
                                     UInt64 lba,
                                     UInt64 count)
{
    if(lba > m->blocks || count > m->blocks - lba)
    {
        return scsi_fail(m, SK_ILLEGAL_REQUEST, 0x21, 0);
    }
    if(write && m->readonly)
    {
        return scsi_fail(m, SK_DATA_PROTECT, 0x27, 0);
    }
    if(count * MSC_BLOCK_SIZE > 0xFFFFFFFFULL)
    {
        return scsi_fail(m, SK_ILLEGAL_REQUEST, 0x24, 0);
    }

    msc_sense(m, SK_NO_SENSE, 0, 0);
    m->source = SRC_DISK;
    m->disk_offset = lba * MSC_BLOCK_SIZE;
    if(count == 0)
    {
        return (scsi_result_t){ DIR_NONE, 0, true };
    }
    return (scsi_result_t){ write ? DIR_OUT : DIR_IN, (UInt32)(count * MSC_BLOCK_SIZE), true };
}

static scsi_result_t scsi_execute(msc_t *m,
                                  const UInt8 *cdb,
                                  UInt8 cdb_length)
{
    UInt8 *b = m->buffer;
    memset(b, 0, sizeof(m->buffer));

    switch(cdb[0])
    {
        case 0x00:  /* TEST UNIT READY */
        case 0x1B:  /* START STOP UNIT */
        case 0x1E:  /* PREVENT ALLOW MEDIUM REMOVAL */
        case 0x2F:  /* VERIFY(10) */
            msc_sense(m, SK_NO_SENSE, 0, 0);
            return (scsi_result_t){ DIR_NONE, 0, true };

        case 0x35:  /* SYNCHRONIZE CACHE(10) */
        case 0x91:  /* SYNCHRONIZE CACHE(16) */
            if(!m->readonly && fsync(m->fd) != 0)
            {
                return scsi_fail(m, SK_MEDIUM_ERROR, 0x0C, 0);
            }
            msc_sense(m, SK_NO_SENSE, 0, 0);
            return (scsi_result_t){ DIR_NONE, 0, true };

        case 0x03:  /* REQUEST SENSE */
            b[0] = 0x70;
            b[2] = m->sense_key;
            b[7] = 10;
            b[12] = m->asc;
            b[13] = m->ascq;
            return scsi_data_in(m, 18, cdb[4]);

        case 0x12:  /* INQUIRY */
        {
            UInt32 alloc = be16(cdb + 3);
            if(cdb[1] & 1)
            {
                /* vital product data */
                if(cdb[2] == 0x00)
                {
                    b[1] = 0x00;
                    b[3] = 2;
                    b[4] = 0x00;
                    b[5] = 0x80;
                    return scsi_data_in(m, 6, alloc);
                }
                if(cdb[2] == 0x80)
                {
                    b[1] = 0x80;
                    b[3] = 16;
                    memcpy(b + 4, m->serial, 16);
                    return scsi_data_in(m, 20, alloc);
                }
                return scsi_fail(m, SK_ILLEGAL_REQUEST, 0x24, 0);
            }
            if(cdb[2] != 0)
            {
                return scsi_fail(m, SK_ILLEGAL_REQUEST, 0x24, 0);  /* invalid field in CDB */
            }
            b[0] = 0x00;
            b[1] = 0x80;
            b[2] = 0x06;
            b[3] = 0x02;
            b[4] = 31;
            memcpy(b + 8, "emexLabs", 8);
            memcpy(b + 16, "emex64 USB Disk ", 16);
            memcpy(b + 32, "1.0 ", 4);
            return scsi_data_in(m, 36, alloc);
        }

        case 0x1A:  /* MODE SENSE(6) */
            b[0] = 3;
            b[2] = m->readonly ? 0x80 : 0;
            return scsi_data_in(m, 4, cdb[4]);

        case 0x5A:  /* MODE SENSE(10) */
            b[1] = 6;
            b[3] = m->readonly ? 0x80 : 0;
            return scsi_data_in(m, 8, be16(cdb + 7));

        case 0x25:  /* READ CAPACITY(10) */
            put_be32(b, m->blocks - 1 > 0xFFFFFFFFULL ? 0xFFFFFFFFu : (UInt32)(m->blocks - 1));
            put_be32(b + 4, MSC_BLOCK_SIZE);
            return scsi_data_in(m, 8, 8);

        case 0x9E:  /* SERVICE ACTION IN(16) */
            if((cdb[1] & 0x1F) != 0x10)
            {
                return scsi_fail(m, SK_ILLEGAL_REQUEST, 0x24, 0);
            }
            /* READ CAPACITY(16) */
            put_be64(b, m->blocks - 1);
            put_be32(b + 8, MSC_BLOCK_SIZE);
            return scsi_data_in(m, 32, be32(cdb + 10));

        case 0x28:  /* READ(10) */
        case 0x2A:  /* WRITE(10) */
            return scsi_read_write(m, cdb[0] == 0x2A, be32(cdb + 2), be16(cdb + 7));

        case 0x88:  /* READ(16) */
        case 0x8A:  /* WRITE(16) */
            if(cdb_length < 16)
            {
                return scsi_fail(m, SK_ILLEGAL_REQUEST, 0x24, 0);
            }
            return scsi_read_write(m, cdb[0] == 0x8A, be64(cdb + 2), be32(cdb + 10));

        default:
            return scsi_fail(m, SK_ILLEGAL_REQUEST, 0x20, 0);  /* invalid command operation code */
    }
}

static void msc_halt(msc_t *m,
                     Boolean in)
{
    if(in)
    {
        m->usb.halted_in |= (UInt16)(1u << MSC_EP_IN);
    }
    else
    {
        m->usb.halted_out |= (UInt16)(1u << MSC_EP_OUT);
    }
}

static void msc_to_status(msc_t *m,
                          UInt8 status)
{
    m->status = status;
    m->done = 0;
    m->transfer = 0;
    m->state = MSC_CSW;
}

static UInt8 msc_command(msc_t *m,
                         const UInt8 *cbw,
                         UInt64 len)
{
    if(len != CBW_SIZE || le32(cbw) != CBW_SIGNATURE || cbw[14] == 0 || cbw[14] > 16 || (cbw[12] & 0x7F) != 0)
    {
        m->needs_reset = true;
        msc_halt(m, true);
        return USB_ERR_STALL;
    }

    m->tag = le32(cbw + 4);
    m->host_length = le32(cbw + 8);
    Boolean host_in = (cbw[12] & 0x80) != 0;
    m->done = 0;
    m->zlp = false;

    scsi_result_t r;
    if((cbw[13] & 0x0F) != 0)
    {
        r = scsi_fail(m, SK_ILLEGAL_REQUEST, 0x25, 0);
    }
    else
    {
        r = scsi_execute(m, cbw + 15, cbw[14]);
    }

    UInt8 status = r.ok ? CSW_PASSED : CSW_FAILED;

    if(r.dir == DIR_NONE)
    {
        if(m->host_length != 0)
        {
            msc_halt(m, host_in);
        }
        msc_to_status(m, status);
        return USB_OK;
    }

    Boolean dev_in = r.dir == DIR_IN;
    if(m->host_length == 0 || host_in != dev_in)
    {
        if(m->host_length != 0)
        {
            msc_halt(m, host_in);
        }
        msc_to_status(m, CSW_PHASE_ERROR);
        return USB_OK;
    }

    if(dev_in)
    {
        m->transfer = r.length < m->host_length ? r.length : m->host_length;
        m->zlp = r.length < m->host_length;
        m->status = r.length > m->host_length ? CSW_PHASE_ERROR : status;
        m->state = MSC_DATA_IN;
    }
    else
    {
        m->transfer = m->host_length;
        if(r.length > m->host_length)
        {
            m->process = 0;
            m->source = SRC_DISCARD;
            m->status = CSW_PHASE_ERROR;
        }
        else
        {
            m->process = r.length;
            m->status = status;
        }
        m->state = MSC_DATA_OUT;
    }
    return USB_OK;
}

static UInt8 msc_bulk_in(emex64_usb_device_t *dev,
                         UInt8 ep,
                         UInt8 *data,
                         UInt64 len,
                         UInt64 *actual)
{
    msc_t *m = dev->context;
    *actual = 0;

    if(ep != MSC_EP_IN)
    {
        return USB_ERR_STALL;
    }

    switch(m->state)
    {
        case MSC_DATA_IN:
        {
            UInt64 n = m->transfer - m->done;
            if(n > len)
            {
                n = len;
            }
            if(n == 0 && m->zlp)
            {
                m->state = MSC_CSW;
                return USB_OK;
            }
            if(m->source == SRC_BUFFER)
            {
                memcpy(data, m->buffer + m->done, n);
            }
            else
            {
                UInt64 got = 0;
                while(got < n)
                {
                    ssize_t r = pread(m->fd, data + got, n - got, (off_t)(m->disk_offset + m->done + got));
                    if(r <= 0)
                    {
                        m->done += (UInt32)got;
                        *actual = got;
                        msc_sense(m, SK_MEDIUM_ERROR, 0x11, 0);
                        m->status = CSW_FAILED;
                        m->state = MSC_CSW;
                        return USB_ERR_STALL;
                    }
                    got += (UInt64)r;
                }
            }
            m->done += (UInt32)n;
            *actual = n;
            if(m->done == m->transfer && (n < len || !m->zlp))
            {
                m->state = MSC_CSW;
            }
            return USB_OK;
        }

        case MSC_CSW:
        {
            if(len < CSW_SIZE)
            {
                return USB_ERR_BABBLE;
            }
            put_le32(data, CSW_SIGNATURE);
            put_le32(data + 4, m->tag);
            put_le32(data + 8, m->host_length - m->done);
            data[12] = m->status;
            *actual = CSW_SIZE;
            m->state = MSC_CBW;
            return USB_OK;
        }

        default:
            return USB_ERR_NAK;
    }
}

static UInt8 msc_bulk_out(emex64_usb_device_t *dev,
                          UInt8 ep,
                          const UInt8 *data,
                          UInt64 len,
                          UInt64 *actual)
{
    msc_t *m = dev->context;
    *actual = 0;

    if(ep != MSC_EP_OUT)
    {
        return USB_ERR_STALL;
    }

    switch(m->state)
    {
        case MSC_CBW:
        {
            UInt8 err = msc_command(m, data, len);
            if(err == USB_OK)
            {
                *actual = len;
            }
            return err;
        }

        case MSC_DATA_OUT:
        {
            UInt64 n = m->transfer - m->done;
            if(n > len)
            {
                n = len;
            }
            UInt64 use = 0;
            if(m->done < m->process)
            {
                use = m->process - m->done;
                if(use > n)
                {
                    use = n;
                }
            }
            if(use != 0 && m->source == SRC_DISK && m->status == CSW_PASSED)
            {
                UInt64 put = 0;
                while(put < use)
                {
                    ssize_t r = pwrite(m->fd, data + put, use - put, (off_t)(m->disk_offset + m->done + put));
                    if(r <= 0)
                    {
                        msc_sense(m, SK_MEDIUM_ERROR, 0x0C, 0);
                        m->status = CSW_FAILED;
                        break;
                    }
                    put += (UInt64)r;
                }
            }
            m->done += (UInt32)n;
            *actual = n;
            if(m->done == m->transfer)
            {
                m->done = m->process;
                m->state = MSC_CSW;
            }
            return USB_OK;
        }

        default:
            return USB_ERR_NAK;
    }
}

static UInt8 msc_control(emex64_usb_device_t *dev,
                         const UInt8 setup[8],
                         UInt8 *data,
                         UInt32 len,
                         UInt32 *actual)
{
    msc_t *m = dev->context;
    UInt16 value = setup[2] | (setup[3] << 8);
    UInt16 index = setup[4] | (setup[5] << 8);

    if((setup[0] & USB_TYPE_MASK) != USB_TYPE_CLASS ||
       (setup[0] & USB_RECIP_MASK) != USB_RECIP_INTERFACE ||
       index != 0 ||
       dev->configuration == 0)
    {
        return USB_ERR_STALL;
    }

    switch(setup[1])
    {
        case MSC_REQ_RESET:
            if(setup[0] != 0x21 || value != 0 || len != 0)
            {
                return USB_ERR_STALL;
            }
            m->state = MSC_CBW;
            m->needs_reset = false;
            return USB_OK;

        case MSC_REQ_GET_MAX_LUN:
            if(setup[0] != 0xA1 || value != 0 || len < 1)
            {
                return USB_ERR_STALL;
            }
            data[0] = 0;
            *actual = 1;
            return USB_OK;

        default:
            return USB_ERR_STALL;
    }
}

static void msc_configured(emex64_usb_device_t *dev,
                           UInt8 configuration)
{
    msc_t *m = dev->context;
    m->state = MSC_CBW;
    m->needs_reset = false;
}

static Boolean msc_clear_halt(emex64_usb_device_t *dev,
                              UInt8 ep_address)
{
    msc_t *m = dev->context;
    return !m->needs_reset;
}

static void msc_free(emex64_usb_device_t *dev)
{
    msc_t *m = dev->context;
    if(m->fd >= 0)
    {
        close(m->fd);
    }
    free(m);
}

static const emex64_usb_device_ops_t msc_ops = {
    .control = msc_control,
    .bulk_in = msc_bulk_in,
    .bulk_out = msc_bulk_out,
    .configured = msc_configured,
    .clear_halt = msc_clear_halt,
    .free = msc_free,
};

emex64_usb_device_t *emex64_usb_msc_alloc(const char *path,
                                          Boolean readonly,
                                          UInt8 index)
{
    int fd = open(path, readonly ? O_RDONLY : O_RDWR);
    if(fd < 0)
    {
        diag_error(NULL, "usb: cannot open disk image '%s'", path);
        return NULL;
    }

    struct stat st;
    if(fstat(fd, &st) != 0 || st.st_size < MSC_BLOCK_SIZE)
    {
        diag_error(NULL, "usb: disk image '%s' is smaller than one block", path);
        close(fd);
        return NULL;
    }

    msc_t *m = calloc(1, sizeof(msc_t));
    if(m == NULL)
    {
        close(fd);
        return NULL;
    }

    m->fd = fd;
    m->readonly = readonly;
    m->blocks = (UInt64)st.st_size / MSC_BLOCK_SIZE;
    snprintf(m->serial, sizeof(m->serial), "E64%02X%011llX", index, (unsigned long long)(m->blocks & 0xFFFFFFFFFFFULL));

    static const UInt8 device_descriptor[18] = {
        18, USB_DT_DEVICE,
        0x00, 0x02,
        0x00, 0x00, 0x00,
        64,
        0x40, 0xE6,
        0x02, 0x00,
        0x00, 0x01,
        1, 2, 3,
        1,
    };
    static const UInt8 config_descriptor[32] = {
        9, USB_DT_CONFIG, 32, 0, 1, 1, 0, 0x80, 50,
        9, USB_DT_INTERFACE, 0, 0, 2, 0x08, 0x06, 0x50, 0,
        7, USB_DT_ENDPOINT, USB_DIR_IN | MSC_EP_IN, USB_EP_BULK, 0x00, 0x02, 0,
        7, USB_DT_ENDPOINT, MSC_EP_OUT, USB_EP_BULK, 0x00, 0x02, 0,
    };
    memcpy(m->device_descriptor, device_descriptor, sizeof(device_descriptor));
    memcpy(m->config_descriptor, config_descriptor, sizeof(config_descriptor));

    m->usb.ops = &msc_ops;
    m->usb.context = m;
    m->usb.speed = USB_SPEED_HIGH;
    m->usb.device_descriptor = m->device_descriptor;
    m->usb.config_descriptor = m->config_descriptor;
    m->usb.config_length = sizeof(config_descriptor);
    m->usb.strings[1] = "emexLabs";
    m->usb.strings[2] = "emex64 USB Disk";
    m->usb.strings[3] = m->serial;
    m->state = MSC_CBW;

    return &m->usb;
}

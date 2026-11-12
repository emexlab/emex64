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

#ifndef EMEX64VM_DEVICE_NET_H
#define EMEX64VM_DEVICE_NET_H

#include <pthread.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64MachineOptions.h>
#include <EmexToolchain/VM/device/board/pcie/pcie.h>

#define NET_PCI_DEVICE_ID           0x0004
#define NET_PCI_CLASS               0x0200E6
#define NET_BAR0_SIZE               EMEX64_PAGE_SIZE

#define NET_REG_CAP                 0x00
#define NET_REG_CTRL                0x08
#define NET_REG_STATUS              0x10
#define NET_REG_MAC                 0x18
#define NET_REG_IS                  0x20
#define NET_REG_IE                  0x28
#define NET_REG_TX_BASE             0x40
#define NET_REG_TX_COUNT            0x48
#define NET_REG_TX_HEAD             0x50
#define NET_REG_TX_TAIL             0x58
#define NET_REG_RX_BASE             0x60
#define NET_REG_RX_COUNT            0x68
#define NET_REG_RX_HEAD             0x70
#define NET_REG_RX_TAIL             0x78
#define NET_REG_DROPS               0x80

#define NET_CTRL_RX                 (1ULL << 0)
#define NET_CTRL_TX                 (1ULL << 1)
#define NET_CTRL_RESET              (1ULL << 2)

#define NET_STATUS_LINK             (1ULL << 0)

#define NET_IS_RX                   (1ULL << 0)
#define NET_IS_TX                   (1ULL << 1)

#define NET_DESC_SIZE               32
#define NET_DESC_ADDR               0x00
#define NET_DESC_LEN                0x08
#define NET_DESC_STATUS             0x10

#define NET_DESC_DONE               (1ULL << 0)
#define NET_DESC_ERROR              (1ULL << 1)

#define NET_MAX_FRAME               1514
#define NET_MAX_RING                4096
#define NET_VERSION                 1

typedef struct emex64_net emex64_net_t;

emex64_net_t *emex64_net_alloc(emex64_pcie_t *pcie, UInt8 dev, const E64MachineOptions *options);
void emex64_net_dealloc(emex64_net_t *net);
void emex64_net_reset(emex64_net_t *net);

#endif /* EMEX64VM_DEVICE_NET_H */

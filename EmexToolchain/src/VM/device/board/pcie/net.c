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

#if EMEX64VM_NET

#include <EmexToolchain/VM/device/board/pcie/net.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include <libslirp.h>
#include <EmexToolchain/Support/diagnostic/log.h>

typedef struct net_timer {
    struct net_timer *next;
    SlirpTimerCb cb;
    void *cb_opaque;
    SInt64 expire;
} net_timer_t;

struct emex64_net {
    emex64_pcie_function_t function;
    pthread_mutex_t lock;
    UInt8 mac[6];

    UInt64 ctrl, is, ie, drops;
    UInt64 tx_base, tx_count, tx_head, tx_tail;
    UInt64 rx_base, rx_count, rx_head, rx_tail;

    Slirp *slirp;
    net_timer_t *timers;
    pthread_t thread;
    Boolean thread_started;
    volatile Boolean stop;
    int wake[2];

    struct pollfd *pfds;
    int npfds, maxpfds;
};

static void net_wr64(UInt8 *p,
                     UInt64 v)
{
    for(int i = 0; i < 8; i++)
    {
        p[i] = (UInt8)(v >> (8 * i));
    }
}

static UInt64 net_rd64(const UInt8 *p)
{
    UInt64 v = 0;
    for(int i = 0; i < 8; i++)
    {
        v |= (UInt64)p[i] << (8 * i);
    }
    return v;
}

static SInt64 net_now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (SInt64)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static void net_wake(emex64_net_t *net)
{
    char c = 0;
    ssize_t r = write(net->wake[1], &c, 1);
}

static void net_update_irq(emex64_net_t *net)
{
    if(net->is & net->ie)
    {
        emex64_pcie_raise_irq(&net->function);
    }
}

static void net_reset(emex64_net_t *net)
{
    net->ctrl = 0;
    net->is = 0;
    net->ie = 0;
    net->drops = 0;
    net->tx_base = net->tx_count = net->tx_head = net->tx_tail = 0;
    net->rx_base = net->rx_count = net->rx_head = net->rx_tail = 0;
}

static UInt8 *net_desc(emex64_net_t *net,
                       UInt64 base,
                       UInt64 count,
                       UInt64 n)
{
    if(count == 0 || (count & (count - 1)) != 0 || count > NET_MAX_RING)
    {
        return NULL;
    }
    return emex64_pcie_dma_map(&net->function, base + (n & (count - 1)) * NET_DESC_SIZE, NET_DESC_SIZE, true);
}

static void net_deliver(emex64_net_t *net,
                        const UInt8 *frame,
                        size_t len)
{
    if(!(net->ctrl & NET_CTRL_RX) || net->rx_head == net->rx_tail || len > NET_MAX_FRAME)
    {
        net->drops++;
        return;
    }

    UInt8 *d = net_desc(net, net->rx_base, net->rx_count, net->rx_head);
    if(d == NULL)
    {
        net->drops++;
        return;
    }

    UInt64 addr = net_rd64(d + NET_DESC_ADDR);
    UInt64 size = net_rd64(d + NET_DESC_LEN);
    UInt8 *buf = size >= len ? emex64_pcie_dma_map(&net->function, addr, len, true) : NULL;
    if(buf == NULL)
    {
        net_wr64(d + NET_DESC_LEN, 0);
        net_wr64(d + NET_DESC_STATUS, NET_DESC_DONE | NET_DESC_ERROR);
    }
    else
    {
        memcpy(buf, frame, len);
        net_wr64(d + NET_DESC_LEN, len);
        net_wr64(d + NET_DESC_STATUS, NET_DESC_DONE);
    }

    __atomic_thread_fence(__ATOMIC_RELEASE);
    net->rx_head++;
    net->is |= NET_IS_RX;
    net_update_irq(net);
}

static void net_transmit(emex64_net_t *net)
{
    Boolean sent = false;
    while(net->tx_head != net->tx_tail)
    {
        UInt8 *d = net_desc(net, net->tx_base, net->tx_count, net->tx_head);
        if(d == NULL)
        {
            break;
        }

        UInt64 addr = net_rd64(d + NET_DESC_ADDR);
        UInt64 len = net_rd64(d + NET_DESC_LEN);
        UInt8 *frame = (len >= 14 && len <= NET_MAX_FRAME) ? emex64_pcie_dma_map(&net->function, addr, len, false) : NULL;
        if(frame == NULL)
        {
            net_wr64(d + NET_DESC_STATUS, NET_DESC_DONE | NET_DESC_ERROR);
        }
        else
        {
            if(net->ctrl & NET_CTRL_TX)
            {
                slirp_input(net->slirp, frame, (int)len);
            }
            net_wr64(d + NET_DESC_STATUS, NET_DESC_DONE);
        }

        net->tx_head++;
        sent = true;
    }

    if(sent)
    {
        net->is |= NET_IS_TX;
        net_update_irq(net);
        net_wake(net);
    }
}

static UInt64 net_bar_read(emex64_pcie_function_t *function,
                           UInt8 bar,
                           UInt64 offset)
{
    emex64_net_t *net = function->context;
    if(bar != 0)
    {
        return 0;
    }

    UInt64 v = 0;
    pthread_mutex_lock(&net->lock);
    switch(offset)
    {
        case NET_REG_CAP:
            v = NET_MAX_FRAME | ((UInt64)NET_VERSION << 32);
            break;
        case NET_REG_CTRL:
            v = net->ctrl;
            break;
        case NET_REG_STATUS:
            v = NET_STATUS_LINK;
            break;
        case NET_REG_MAC:
            for(int i = 0; i < 6; i++)
            {
                v |= (UInt64)net->mac[i] << (8 * i);
            }
            break;
        case NET_REG_IS:
            v = net->is;
            break;
        case NET_REG_IE:
            v = net->ie;
            break;
        case NET_REG_TX_BASE:
            v = net->tx_base;
            break;
        case NET_REG_TX_COUNT:
            v = net->tx_count;
            break;
        case NET_REG_TX_HEAD:
            v = net->tx_head;
            break;
        case NET_REG_TX_TAIL:
            v = net->tx_tail;
            break;
        case NET_REG_RX_BASE:
            v = net->rx_base;
            break;
        case NET_REG_RX_COUNT:
            v = net->rx_count;
            break;
        case NET_REG_RX_HEAD:
            v = net->rx_head;
            break;
        case NET_REG_RX_TAIL:
            v = net->rx_tail;
            break;
        case NET_REG_DROPS:
            v = net->drops;
            break;
        default:
            break;
    }
    pthread_mutex_unlock(&net->lock);
    return v;
}

static void net_bar_write(emex64_pcie_function_t *function,
                          UInt8 bar,
                          UInt64 offset,
                          UInt64 value)
{
    emex64_net_t *net = function->context;
    if(bar != 0)
    {
        return;
    }

    pthread_mutex_lock(&net->lock);
    switch(offset)
    {
        case NET_REG_CTRL:
            if(value & NET_CTRL_RESET)
            {
                net_reset(net);
            }
            else
            {
                net->ctrl = value & (NET_CTRL_RX | NET_CTRL_TX);
            }
            break;
        case NET_REG_IS:
            net->is &= ~value;
            break;
        case NET_REG_IE:
            net->ie = value & (NET_IS_RX | NET_IS_TX);
            net_update_irq(net);
            break;
        case NET_REG_TX_BASE:
            net->tx_base = value & ~(UInt64)(NET_DESC_SIZE - 1);
            break;
        case NET_REG_TX_COUNT:
            net->tx_count = value;
            net->tx_head = net->tx_tail = 0;
            break;
        case NET_REG_TX_TAIL:
            /* at most a ring ahead of what was sent */
            if(value - net->tx_head <= net->tx_count)
            {
                net->tx_tail = value;
                net_transmit(net);
            }
            break;
        case NET_REG_RX_BASE:
            net->rx_base = value & ~(UInt64)(NET_DESC_SIZE - 1);
            break;
        case NET_REG_RX_COUNT:
            net->rx_count = value;
            net->rx_head = net->rx_tail = 0;
            break;
        case NET_REG_RX_TAIL:
            if(value - net->rx_head <= net->rx_count)
            {
                net->rx_tail = value;
            }
            break;
        default:
            break;
    }
    pthread_mutex_unlock(&net->lock);
}

static const emex64_pcie_ops_t net_ops = {
    .bar_read = net_bar_read,
    .bar_write = net_bar_write,
};

/* libslirp type shii */
static ssize_t net_slirp_send(const void *buf,
                              size_t len,
                              void *opaque)
{
    net_deliver(opaque, buf, len);
    return (ssize_t)len;
}

static void net_slirp_guest_error(const char *msg,
                                  void *opaque)
{
    diag_warn(NULL, "net: %s", msg);
}

static int64_t net_slirp_clock(void *opaque)
{
    return net_now_ns();
}

static void *net_slirp_timer_new(SlirpTimerCb cb,
                                 void *cb_opaque,
                                 void *opaque)
{
    emex64_net_t *net = opaque;
    net_timer_t *t = calloc(1, sizeof(net_timer_t));
    if(t == NULL)
    {
        return NULL;
    }
    t->cb = cb;
    t->cb_opaque = cb_opaque;
    t->expire = -1;
    t->next = net->timers;
    net->timers = t;
    return t;
}

static void net_slirp_timer_free(void *timer,
                                 void *opaque)
{
    emex64_net_t *net = opaque;
    for(net_timer_t **p = &net->timers; *p != NULL; p = &(*p)->next)
    {
        if(*p == timer)
        {
            *p = ((net_timer_t *)timer)->next;
            free(timer);
            return;
        }
    }
}

static void net_slirp_timer_mod(void *timer,
                                int64_t expire,
                                void *opaque)
{
    ((net_timer_t *)timer)->expire = expire;
    net_wake(opaque);
}

static void net_slirp_register_fd(int fd, void *opaque)
{
}

static void net_slirp_notify(void *opaque)
{
    net_wake(opaque);
}

static const SlirpCb net_slirp_cb = {
    .send_packet = net_slirp_send,
    .guest_error = net_slirp_guest_error,
    .clock_get_ns = net_slirp_clock,
    .timer_new = net_slirp_timer_new,
    .timer_free = net_slirp_timer_free,
    .timer_mod = net_slirp_timer_mod,
    .register_poll_fd = net_slirp_register_fd,
    .unregister_poll_fd = net_slirp_register_fd,
    .notify = net_slirp_notify,
};

static int net_add_poll(int fd,
                        int events,
                        void *opaque)
{
    emex64_net_t *net = opaque;
    if(net->npfds == net->maxpfds)
    {
        int n = net->maxpfds ? net->maxpfds * 2 : 32;
        struct pollfd *p = realloc(net->pfds, sizeof(struct pollfd) * n);
        if(p == NULL)
        {
            return -1;
        }
        net->pfds = p;
        net->maxpfds = n;
    }

    short ev = 0;
    if(events & SLIRP_POLL_IN)
    {
        ev |= POLLIN;
    }
    if(events & SLIRP_POLL_OUT)
    {
        ev |= POLLOUT;
    }
    if(events & SLIRP_POLL_PRI)
    {
        ev |= POLLPRI;
    }

    net->pfds[net->npfds].fd = fd;
    net->pfds[net->npfds].events = ev;
    net->pfds[net->npfds].revents = 0;
    return net->npfds++;
}

static int net_get_revents(int idx,
                           void *opaque)
{
    emex64_net_t *net = opaque;
    if(idx < 0 || idx >= net->npfds)
    {
        return 0;
    }

    short r = net->pfds[idx].revents;
    int ev = 0;
    if(r & POLLIN)
    {
        ev |= SLIRP_POLL_IN;
    }
    if(r & POLLOUT)
    {
        ev |= SLIRP_POLL_OUT;
    }
    if(r & POLLPRI)
    {
        ev |= SLIRP_POLL_PRI;
    }
    if(r & POLLERR)
    {
        ev |= SLIRP_POLL_ERR;
    }
    if(r & POLLHUP)
    {
        ev |= SLIRP_POLL_HUP;
    }

    return ev;
}

static void net_run_timers(emex64_net_t *net)
{
    SInt64 now = net_now_ns() / 1000000;
    Boolean again = true;
    while(again)
    {
        again = false;
        for(net_timer_t *t = net->timers; t != NULL; t = t->next)
        {
            if(t->expire >= 0 && t->expire <= now)
            {
                t->expire = -1;
                t->cb(t->cb_opaque);
                again = true;
                break;
            }
        }
    }
}

static void *net_thread(void *arg)
{
    emex64_net_t *net = arg;
    while(!net->stop)
    {
        pthread_mutex_lock(&net->lock);
        net->npfds = 0;
        net_add_poll(net->wake[0], SLIRP_POLL_IN, net);

        uint32_t timeout = 1000;
        SInt64 now = net_now_ns() / 1000000;
        for(net_timer_t *t = net->timers; t != NULL; t = t->next)
        {
            if(t->expire >= 0)
            {
                SInt64 left = t->expire > now ? t->expire - now : 0;
                if(left < (SInt64)timeout)
                {
                    timeout = (uint32_t)left;
                }
            }
        }

        slirp_pollfds_fill(net->slirp, &timeout, net_add_poll, net);
        pthread_mutex_unlock(&net->lock);

        int ret = poll(net->pfds, (nfds_t)net->npfds, (int)timeout);
        if(ret > 0 && (net->pfds[0].revents & POLLIN))
        {
            char buf[64];
            while(read(net->wake[0], buf, sizeof(buf)) > 0)
            {
            }
        }

        pthread_mutex_lock(&net->lock);
        slirp_pollfds_poll(net->slirp, ret < 0, net_get_revents, net);
        net_run_timers(net);
        pthread_mutex_unlock(&net->lock);
    }
    return NULL;
}

emex64_net_t *emex64_net_alloc(emex64_pcie_t *pcie,
                               UInt8 dev,
                               const E64MachineOptions *options)
{
    emex64_net_t *net = calloc(1, sizeof(emex64_net_t));
    if(net == NULL)
    {
        return NULL;
    }
    pthread_mutex_init(&net->lock, NULL);
    net->wake[0] = net->wake[1] = -1;
    const UInt8 mac[6] = { 0x02, 0xE6, 0x40, 0x00, 0x00, 0x01 };
    memcpy(net->mac, mac, sizeof(mac));

    net->function.vendor = PCIE_VENDOR_EMEXLABS;
    net->function.device = NET_PCI_DEVICE_ID;
    net->function.class_code = NET_PCI_CLASS;
    net->function.revision = 1;
    net->function.bar_size[0] = NET_BAR0_SIZE;
    net->function.ops = &net_ops;
    net->function.context = net;
    net_reset(net);

    if(pipe(net->wake) != 0)
    {
        emex64_net_dealloc(net);
        return NULL;
    }
    fcntl(net->wake[0], F_SETFL, O_NONBLOCK);
    fcntl(net->wake[1], F_SETFL, O_NONBLOCK);

    SlirpConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.version = 1;
    cfg.in_enabled = true;
    inet_pton(AF_INET, "10.0.2.0", &cfg.vnetwork);
    inet_pton(AF_INET, "255.255.255.0", &cfg.vnetmask);
    inet_pton(AF_INET, "10.0.2.2", &cfg.vhost);
    inet_pton(AF_INET, "10.0.2.15", &cfg.vdhcp_start);
    inet_pton(AF_INET, "10.0.2.3", &cfg.vnameserver);
    cfg.vhostname = "emex64";
    net->slirp = slirp_new(&cfg, &net_slirp_cb, net);
    if(net->slirp == NULL)
    {
        diag_error(NULL, "net: user mode networking could not start");
        emex64_net_dealloc(net);
        return NULL;
    }

    struct in_addr host, guest;
    inet_pton(AF_INET, "127.0.0.1", &host);
    inet_pton(AF_INET, "10.0.2.15", &guest);
    for(UInt8 i = 0; i < options->networkOptions.forwardCount && i < E64_MACHINE_MAX_HOST_FORWARDS; i++)
    {
        const E64HostForward *f = &options->networkOptions.forwards[i];
        if(slirp_add_hostfwd(net->slirp, f->udp, host, f->hostPort, guest, f->guestPort) < 0)
        {
            diag_warn(NULL, "net: host port %u is taken: not forwarded to the guest's %u (%s)",
                      f->hostPort, f->guestPort, strerror(errno));
        }
        else
        {
            fprintf(stderr, "emex64vm: %s localhost:%u -> guest port %u\n", f->udp ? "udp" : "tcp", f->hostPort, f->guestPort);
        }
    }

    if(!emex64_pcie_attach(pcie, &net->function, dev, 0))
    {
        emex64_net_dealloc(net);
        return NULL;
    }

    if(pthread_create(&net->thread, NULL, net_thread, net) != 0)
    {
        emex64_net_dealloc(net);
        return NULL;
    }

    net->thread_started = true;
    return net;
}

void emex64_net_reset(emex64_net_t *net)
{
    if(net == NULL)
    {
        return;
    }
    pthread_mutex_lock(&net->lock);
    net_reset(net);
    pthread_mutex_unlock(&net->lock);
}

void emex64_net_dealloc(emex64_net_t *net)
{
    if(net == NULL)
    {
        return;
    }

    if(net->thread_started)
    {
        net->stop = true;
        net_wake(net);
        pthread_join(net->thread, NULL);
    }
    if(net->slirp != NULL)
    {
        slirp_cleanup(net->slirp);
    }
    while(net->timers != NULL)
    {
        net_timer_t *t = net->timers;
        net->timers = t->next;
        free(t);
    }
    if(net->wake[0] >= 0)
    {
        close(net->wake[0]);
        close(net->wake[1]);
    }

    free(net->pfds);
    pthread_mutex_destroy(&net->lock);
    free(net);
}

#else

emex64_net_t *emex64_net_alloc(emex64_pcie_t *pcie,
                               UInt8 dev,
                               const E64MachineOptions *options)
{
    return NULL;
}

void emex64_net_dealloc(emex64_net_t *net)
{
}

void emex64_net_reset(emex64_net_t *net)
{
}

#endif /* EMEX64VM_NET */

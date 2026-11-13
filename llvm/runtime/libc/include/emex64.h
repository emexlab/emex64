#ifndef _EMEX64_H
#define _EMEX64_H

#define EMEX64_PAGE_SIZE        0x2000UL
#define EMEX64_MMIO_BASE        0x0020000000000000UL

#define EMEX64_IC_BASE          (EMEX64_MMIO_BASE + EMEX64_PAGE_SIZE * 0)
#define EMEX64_APIC_BASE        (EMEX64_MMIO_BASE + EMEX64_PAGE_SIZE * 1)
#define EMEX64_TIMER_BASE       (EMEX64_MMIO_BASE + EMEX64_PAGE_SIZE * 2)
#define EMEX64_RTC_BASE         (EMEX64_MMIO_BASE + EMEX64_PAGE_SIZE * 3)
#define EMEX64_UART_BASE        (EMEX64_MMIO_BASE + EMEX64_PAGE_SIZE * 4)
#define EMEX64_MC_BASE          (EMEX64_MMIO_BASE + EMEX64_PAGE_SIZE * 5)
#define EMEX64_PLATFORM_BASE    (EMEX64_MMIO_BASE + EMEX64_PAGE_SIZE * 6)
#define EMEX64_8042_BASE        (EMEX64_MMIO_BASE + EMEX64_PAGE_SIZE * 7)

#define EMEX64_UART_REG_DATA    (EMEX64_UART_BASE + 0x00)
#define EMEX64_UART_REG_STATUS  (EMEX64_UART_BASE + 0x08)
#define EMEX64_UART_REG_CONTROL (EMEX64_UART_BASE + 0x10)

#define EMEX64_UART_STATUS_RX_READY 0x01
#define EMEX64_UART_STATUS_TX_EMPTY 0x02

#define EMEX64_PLATFORM_REG_PWR EMEX64_PLATFORM_BASE

static inline unsigned long emex64_mmio_read(unsigned long addr)
{
    return *(volatile unsigned long *)addr;
}

static inline void emex64_mmio_write(unsigned long addr,
                                     unsigned long value)
{
    *(volatile unsigned long *)addr = value;
}

#endif /* _EMEX64_H */

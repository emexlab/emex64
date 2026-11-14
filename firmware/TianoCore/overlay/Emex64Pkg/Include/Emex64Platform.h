/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef EMEX64_PLATFORM_H_
#define EMEX64_PLATFORM_H_

#define EMEX64_PAGE_SIZE                0x2000ULL
#define EMEX64_MMIO_BASE                0x0020000000000000ULL
#define EMEX64_MMIO_PAGE(n)             (EMEX64_MMIO_BASE + EMEX64_PAGE_SIZE * (n))

/* Interrupt Controller */
#define EMEX64_IC_BASE                  EMEX64_MMIO_PAGE(0)

/* Advanced Programmable Interrupt Controller*/
#define EMEX64_APIC_BASE                EMEX64_MMIO_PAGE(1)
#define EMEX64_APIC_ID                  (EMEX64_APIC_BASE + 0x00)
#define EMEX64_APIC_VERSION             (EMEX64_APIC_BASE + 0x08)
#define EMEX64_APIC_TIMER_CTRL          (EMEX64_APIC_BASE + 0x40)
#define EMEX64_APIC_TIMER_COUNT         (EMEX64_APIC_BASE + 0x48)
#define EMEX64_APIC_TIMER_COMPARE       (EMEX64_APIC_BASE + 0x50)
#define EMEX64_APIC_TIMER_STATUS        (EMEX64_APIC_BASE + 0x58)
#define EMEX64_APIC_TIMER_FREQ          (EMEX64_APIC_BASE + 0x60)

/* Timers */
#define EMEX64_TIMER_BASE               EMEX64_MMIO_PAGE(2)
#define EMEX64_TIMER_CTRL               (EMEX64_TIMER_BASE + 0x00)
#define EMEX64_TIMER_COUNT              (EMEX64_TIMER_BASE + 0x08)
#define EMEX64_TIMER_COMPARE            (EMEX64_TIMER_BASE + 0x10)
#define EMEX64_TIMER_STATUS             (EMEX64_TIMER_BASE + 0x18)
#define EMEX64_TIMER_FREQ               (EMEX64_TIMER_BASE + 0x20)
#define EMEX64_TIMER_CTRL_ENABLE        BIT0
#define EMEX64_TIMER_CTRL_IRQ           BIT1
#define EMEX64_TIMER_CTRL_PERIODIC      BIT2
#define EMEX64_TIMER_STATUS_IRQ         BIT0

/* Real Time Clock */
#define EMEX64_RTC_BASE                 EMEX64_MMIO_PAGE(3)
#define EMEX64_RTC_SECONDS              (EMEX64_RTC_BASE + 0x00)
#define EMEX64_RTC_MINUTES              (EMEX64_RTC_BASE + 0x08)
#define EMEX64_RTC_HOURS                (EMEX64_RTC_BASE + 0x10)
#define EMEX64_RTC_DAY                  (EMEX64_RTC_BASE + 0x18)
#define EMEX64_RTC_MONTH                (EMEX64_RTC_BASE + 0x20)
#define EMEX64_RTC_YEAR                 (EMEX64_RTC_BASE + 0x28)
#define EMEX64_RTC_WEEKDAY              (EMEX64_RTC_BASE + 0x30)

/* Universal Asynchronous Receiver Transmitter */
#define EMEX64_UART_BASE                EMEX64_MMIO_PAGE(4)
#define EMEX64_UART_DATA                (EMEX64_UART_BASE + 0x00)
#define EMEX64_UART_STATUS              (EMEX64_UART_BASE + 0x08)
#define EMEX64_UART_CONTROL             (EMEX64_UART_BASE + 0x10)
#define EMEX64_UART_STATUS_RX_READY     BIT0
#define EMEX64_UART_STATUS_TX_EMPTY     BIT1

/* Memory Controller */
#define EMEX64_MC_BASE                  EMEX64_MMIO_PAGE(5)
#define EMEX64_MC_SIZE                  (EMEX64_MC_BASE + 0x00)
#define EMEX64_MC_KTRR_SIZE             (EMEX64_MC_BASE + 0x08)
#define EMEX64_MC_KTRR_LOCKED           (EMEX64_MC_BASE + 0x10)
#define EMEX64_MC_KTRR_BASE             (EMEX64_MC_BASE + 0x18)

/* Platform Device */
#define EMEX64_PLATFORM_BASE            EMEX64_MMIO_PAGE(6)
#define EMEX64_PLATFORM_POWER           (EMEX64_PLATFORM_BASE + 0x00)

/* 8042 */
#define EMEX64_8042_BASE                EMEX64_MMIO_PAGE(7)
#define EMEX64_8042_DATA                (EMEX64_8042_BASE + 0x00)
#define EMEX64_8042_STATUS              (EMEX64_8042_BASE + 0x08)
#define EMEX64_8042_STATUS_OBF          BIT0
#define EMEX64_8042_STATUS_AUX          BIT5

/* Peripheral Component Interconnect Express Root Complex */
#define EMEX64_PCIE_RC_BASE             EMEX64_MMIO_PAGE(8)
#define EMEX64_PCIE_RC_ID               (EMEX64_PCIE_RC_BASE + 0x00)
#define EMEX64_PCIE_RC_ECAM             (EMEX64_PCIE_RC_BASE + 0x08)
#define EMEX64_PCIE_RC_MMIO_BASE        (EMEX64_PCIE_RC_BASE + 0x18)
#define EMEX64_PCIE_RC_MMIO_SIZE        (EMEX64_PCIE_RC_BASE + 0x20)
#define EMEX64_PCIE_RC_MAGIC            0x49435045ULL
#define EMEX64_PCIE_CFG_ID              0x00
#define EMEX64_PCIE_CFG_CONTROL         0x08
#define EMEX64_PCIE_CFG_BAR(n)          (0x40 + (n) * 0x10)
#define EMEX64_PCIE_CFG_BAR_SIZE(n)     (0x48 + (n) * 0x10)
#define EMEX64_PCIE_CONTROL_MEM         BIT0
#define EMEX64_PCIE_CONTROL_MASTER      BIT1

/* Display Controller*/
#define EMEX64_GPU_VENDOR_ID            0xE640
#define EMEX64_GPU_DEVICE_ID            0x0003
#define EMEX64_GPU_DP_STATUS            0x100
#define EMEX64_GPU_DP_LINK              0x108
#define EMEX64_GPU_DP_AUX_ADDR          0x110
#define EMEX64_GPU_DP_AUX_DATA          0x118
#define EMEX64_GPU_SCAN_CTRL            0x200
#define EMEX64_GPU_SCAN_BASE            0x208
#define EMEX64_GPU_SCAN_WIDTH           0x210
#define EMEX64_GPU_SCAN_HEIGHT          0x218
#define EMEX64_GPU_SCAN_STRIDE          0x220
#define EMEX64_GPU_SCAN_FORMAT          0x228
#define EMEX64_GPU_SCAN_STATUS          0x230
#define EMEX64_GPU_DP_STATUS_HPD        BIT0
#define EMEX64_GPU_DP_STATUS_LINK       BIT1
#define EMEX64_GPU_AUX_EDID             BIT31
#define EMEX64_GPU_FORMAT_XRGB8888      2

/* SATA host controller */
#define EMEX64_SATA_DEVICE_ID           0x0001
#define EMEX64_SATA_CAP                 0x00
#define EMEX64_SATA_GHC                 0x08
#define EMEX64_SATA_PI                  0x18
#define EMEX64_SATA_PORT(p)             (0x100 + (p) * 0x100)
#define EMEX64_SATA_P_SSTS              0x00
#define EMEX64_SATA_P_SECTORS           0x08
#define EMEX64_SATA_P_SECSZ             0x10
#define EMEX64_SATA_P_CLB               0x18
#define EMEX64_SATA_P_CMD               0x20
#define EMEX64_SATA_P_CI                0x28
#define EMEX64_SATA_P_IS                0x30
#define EMEX64_SATA_P_IE                0x38
#define EMEX64_SATA_GHC_RESET           BIT0
#define EMEX64_SATA_SSTS_PRESENT        BIT0
#define EMEX64_SATA_SSTS_READONLY       BIT1
#define EMEX64_SATA_CMD_ST              BIT0
#define EMEX64_SATA_SLOTS               32
#define EMEX64_SATA_SLOT_SIZE           64
#define EMEX64_SATA_READ_DMA_EXT        0x25
#define EMEX64_SATA_WRITE_DMA_EXT       0x35
#define EMEX64_SATA_FLUSH_EXT           0xEA
#define EMEX64_SATA_IDENTIFY            0xEC
#define EMEX64_SATA_STATUS_DONE         BIT0
#define EMEX64_SATA_STATUS_ERROR        BIT1

/* USB host controller */
#define EMEX64_USB_DEVICE_ID            0x0002
#define EMEX64_USB_CAP                  0x00
#define EMEX64_USB_CMD                  0x08
#define EMEX64_USB_STS                  0x10
#define EMEX64_USB_TDLIST               0x18
#define EMEX64_USB_DOORBELL             0x20
#define EMEX64_USB_ERROR                0x28
#define EMEX64_USB_PORTSC(p)            (0x100 + (p) * 0x20)
#define EMEX64_USB_PORTCTL(p)           (0x108 + (p) * 0x20)
#define EMEX64_USB_CMD_RUN              BIT0
#define EMEX64_USB_CMD_RESET            BIT1
#define EMEX64_USB_CMD_IE               BIT2
#define EMEX64_USB_PORTSC_CONNECTED     BIT0
#define EMEX64_USB_PORTSC_ENABLED       BIT1
#define EMEX64_USB_PORTSC_CHANGE        BIT2
#define EMEX64_USB_PORTCTL_RESET        BIT0
#define EMEX64_USB_PORTCTL_DISABLE      BIT1
#define EMEX64_USB_PORTCTL_CLEAR_CHANGE BIT2
#define EMEX64_USB_TD_SLOTS             32
#define EMEX64_USB_TD_CONTROL           1
#define EMEX64_USB_TD_BULK_IN           2
#define EMEX64_USB_TD_BULK_OUT          3
#define EMEX64_USB_TD_DONE              BIT0
#define EMEX64_USB_TD_ERROR             BIT1
#define EMEX64_USB_CC_STALL             1
#define EMEX64_USB_CC_NO_RESPONSE       2
#define EMEX64_USB_CC_BAD_TD            3
#define EMEX64_USB_CC_DMA_FAULT         4
#define EMEX64_USB_CC_BABBLE            5
#define EMEX64_USB_CC_NAK               6

/* Central Processing Unit Identification */
#define EMEX64_CPUID_BASE               EMEX64_MMIO_PAGE (10)
#define EMEX64_CPUID_VENDOR             (EMEX64_CPUID_BASE + 0x00)
#define EMEX64_CPUID_MODEL              (EMEX64_CPUID_BASE + 0x08)
#define EMEX64_CPUID_ISA                (EMEX64_CPUID_BASE + 0x10)
#define EMEX64_CPUID_FEATURES           (EMEX64_CPUID_BASE + 0x18)
#define EMEX64_CPUID_BRAND              (EMEX64_CPUID_BASE + 0x40)

/* Control Registers */
#define EMEX64_CR_CREL                  0
#define EMEX64_CR_CRKSP                 1
#define EMEX64_CR_CREXC                 2
#define EMEX64_CR_CRISA                 6
#define EMEX64_CR_CRSTAT                7
#define EMEX64_CR_CRTVEC                8
#define EMEX64_CR_CREPC                 9
#define EMEX64_CR_CRCAUSE               10
#define EMEX64_CR_CRTVAL                11
#define EMEX64_CR_CRSCRATCH             12
#define EMEX64_CRSTAT_IE                BIT0
#define EMEX64_CRSTAT_PIE               BIT1
#define EMEX64_CRSTAT_TRAPS             BIT16

extern GUID gEmex64DisplayDevicePathGuid;
extern GUID gEmex64KeyboardDevicePathGuid;
extern GUID gEmex64SataDevicePathGuid;
extern GUID gEmex64UsbDevicePathGuid;

#endif /* EMEX64_PLATFORM_H_ */

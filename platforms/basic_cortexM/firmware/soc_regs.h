#ifndef SOC_REGS_H
#define SOC_REGS_H

#include <stdint.h>
#include "soc_memory_map.h"

#define MMIO32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))

#define TIMER_BASE        ((uint32_t)PL_TIMER_BASE)
#define TIMER_REG_CTRL    MMIO32(TIMER_BASE + 0x00)
#define TIMER_REG_VALUE   MMIO32(TIMER_BASE + 0x04)
#define TIMER_REG_CMP     MMIO32(TIMER_BASE + 0x08)
#define TIMER_REG_INTR    MMIO32(TIMER_BASE + 0x0C)
#define TIMER_CTRL_ENABLE (1u << 0)
#define TIMER_CTRL_CMP_EN (1u << 1)
#define TIMER_CTRL_OV_EN  (1u << 2)
#define TIMER_INTR_CMP    (1u << 1)
#define TIMER_INTR_OV     (1u << 2)

#define GPIO_BASE         ((uint32_t)PL_GPIO_BASE)
#define GPIO_REG_DATA     MMIO32(GPIO_BASE + 0x00)
#define GPIO_REG_DIR      MMIO32(GPIO_BASE + 0x04)
#define GPIO_REG_SET      MMIO32(GPIO_BASE + 0x08)
#define GPIO_REG_CLR      MMIO32(GPIO_BASE + 0x0C)
#define GPIO_REG_INTEN    MMIO32(GPIO_BASE + 0x10)
#define GPIO_REG_INTTYPE  MMIO32(GPIO_BASE + 0x14)
#define GPIO_REG_INTPOL   MMIO32(GPIO_BASE + 0x18)
#define GPIO_REG_INTSTAT  MMIO32(GPIO_BASE + 0x1C)
#define GPIO_REG_INTCLR   MMIO32(GPIO_BASE + 0x20)
#define GPIO_REG_ID       MMIO32(GPIO_BASE + 0x24)
#define GPIO_ID_VALUE     0x4750494Fu

#define UART_BASE         ((uint32_t)PL_UART_BASE)
#define UART_DR           MMIO32(UART_BASE + 0x00)
#define UART_FR           MMIO32(UART_BASE + 0x18)
#define UART_IBRD         MMIO32(UART_BASE + 0x24)
#define UART_FBRD         MMIO32(UART_BASE + 0x28)
#define UART_LCRH         MMIO32(UART_BASE + 0x2C)
#define UART_CR           MMIO32(UART_BASE + 0x30)
#define UART_IMSC         MMIO32(UART_BASE + 0x38)
#define UART_RIS          MMIO32(UART_BASE + 0x3C)
#define UART_MIS          MMIO32(UART_BASE + 0x40)
#define UART_ICR          MMIO32(UART_BASE + 0x44)
#define UART_FR_TXFE      (1u << 7)
#define UART_FR_RXFE      (1u << 4)
#define UART_CR_UARTEN    (1u << 0)
#define UART_CR_LBE       (1u << 7)
#define UART_CR_TXE       (1u << 8)
#define UART_CR_RXE       (1u << 9)
#define UART_LCRH_FEN     (1u << 4)
#define UART_INT_RX       (1u << 4)

#define SRAM_BASE         ((uint32_t)PL_SRAM_BASE)

#define SYS_BASE          ((uint32_t)PL_SYSCTRL_BASE)
#define SYS_REG_MAGIC     MMIO32(SYS_BASE + 0x00)
#define SYS_REG_VERSION   MMIO32(SYS_BASE + 0x04)
#define SYS_REG_SCRATCH   MMIO32(SYS_BASE + 0x08)
#define SYS_REG_SYS_HZ    MMIO32(SYS_BASE + 0x0C)
#define SYS_REG_PCLK_HZ   MMIO32(SYS_BASE + 0x10)
#define SYS_REG_STATUS    MMIO32(SYS_BASE + 0x14)
#define SYS_MAGIC_VALUE   0x534F4332u

#define WDT_BASE          ((uint32_t)PL_WDT_BASE)
#define WDT_LOAD          MMIO32(WDT_BASE + 0x00)
#define WDT_VALUE         MMIO32(WDT_BASE + 0x04)
#define WDT_CONTROL       MMIO32(WDT_BASE + 0x08)
#define WDT_INTCLR        MMIO32(WDT_BASE + 0x0C)
#define WDT_RIS           MMIO32(WDT_BASE + 0x10)
#define WDT_LOCK          MMIO32(WDT_BASE + 0xC00)
#define WDT_UNLOCK        0x1ACCE551u
#define WDT_CTRL_INTEN    (1u << 0)

#define NVIC_ISER0        MMIO32(0xE000E100u)

#endif /* SOC_REGS_H */

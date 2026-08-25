#ifndef SOC_MEMORY_MAP_H
#define SOC_MEMORY_MAP_H

/*
 * Shared memory map for QEMU machines and the SystemC Cortex-M PL SoC.
 *
 * systemc-soc  — Cortex-M3 (remote-mmio @ PL base, 1 MiB)
 * systemc-ps   — Cortex-A9 (+ UART, GIC, DDR)
 *
 * Cortex-M PL window (offsets from SYSTEMC_PL_M_PROFILE_BASE):
 *
 *   AHB
 *     +0x00000  Timer     (pin-level IP via AHB pin transactor)
 *     +0x08000  SRAM      (4 KiB)
 *   APB (via AHB-to-APB bridge at +0x10000)
 *     +0x10000  GPIO
 *     +0x11000  UART      (PL011-lite)
 *     +0x12000  SysCtrl
 *     +0x13000  Watchdog
 */

#define SYSTEMC_PL_M_PROFILE_BASE  0x40000000ull
#define SYSTEMC_PL_A_PROFILE_BASE  0xF0000000ull
#define SYSTEMC_PL_WINDOW_SIZE     0x00100000ull

#define PS_DDR_BASE                0x60000000ull
#define PS_GIC_DIST_BASE           0x1e001000ull
#define PS_UART0_BASE              0x10009000ull
#define PS_UART0_IRQ               5

#define M3_FLASH_BASE              0x00000000ull
#define M3_SRAM_BASE               0x20000000ull

#define PL_SLOT_SIZE               0x1000ull
#define PL_TIMER_OFFSET            0x00000ull
#define PL_SRAM_OFFSET             0x08000ull
#define PL_APB_OFFSET              0x10000ull
#define PL_APB_WINDOW_SIZE         0x4000ull
#define PL_GPIO_OFFSET             0x10000ull
#define PL_UART_OFFSET             0x11000ull
#define PL_SYSCTRL_OFFSET          0x12000ull
#define PL_WDT_OFFSET              0x13000ull
#define PL_SRAM_BYTES              0x1000u

#define PL_TIMER_BASE              (SYSTEMC_PL_M_PROFILE_BASE + PL_TIMER_OFFSET)
#define PL_SRAM_BASE               (SYSTEMC_PL_M_PROFILE_BASE + PL_SRAM_OFFSET)
#define PL_GPIO_BASE               (SYSTEMC_PL_M_PROFILE_BASE + PL_GPIO_OFFSET)
#define PL_UART_BASE               (SYSTEMC_PL_M_PROFILE_BASE + PL_UART_OFFSET)
#define PL_SYSCTRL_BASE            (SYSTEMC_PL_M_PROFILE_BASE + PL_SYSCTRL_OFFSET)
#define PL_WDT_BASE                (SYSTEMC_PL_M_PROFILE_BASE + PL_WDT_OFFSET)

/* NVIC IRQ indices on systemc-soc (vector = 16 + n) */
#define PL_IRQ_TIMER_CMP           0
#define PL_IRQ_TIMER_OV            1
#define PL_IRQ_UART                2
#define PL_IRQ_GPIO                3
#define PL_IRQ_WDT                 4
#define PL_IRQ_COUNT               5

#endif /* SOC_MEMORY_MAP_H */

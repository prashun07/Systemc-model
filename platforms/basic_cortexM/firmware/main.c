/*
 * Bare-metal firmware for the Cortex-M production-style PL SoC.
 */

#include <stdint.h>
#include "soc_regs.h"
#include "semihost.h"
#include "log.h"

static volatile uint32_t g_uart_irqs;

void UART_Handler(void)
{
    g_uart_irqs++;
    (void)UART_DR;
    UART_ICR = UART_INT_RX;
}

static void irq_off(void)
{
    __asm volatile ("cpsid i" ::: "memory");
}

static void irq_on(void)
{
    __asm volatile ("cpsie i" ::: "memory");
}

static void mmio_write_named(const char *name, uint32_t off,
                             volatile uint32_t *reg, uint32_t v)
{
    log_mmio_write(name, off, v);
    *reg = v;
}

static int fail(const char *msg)
{
    log_line(msg);
    sh_exit(1);
    return 1;
}

int main(void)
{
    irq_off();

    log_line("========================================");
    log_line("  basic_cortexM — production VP firmware");
    log_line("========================================");
    log_line("[boot] Cortex-M3  machine: systemc-soc");
    sh_puts("[boot] PL ");
    log_put_hex32((uint32_t)SYSTEMC_PL_M_PROFILE_BASE);
    sh_puts(" AHB+APB\n");

    log_line("");
    log_line("[test 0] SysCtrl");
    {
        uint32_t magic = SYS_REG_MAGIC;
        log_mmio_read("MAGIC", 0x00, magic);
        if (magic != SYS_MAGIC_VALUE) {
            return fail("FAIL: SysCtrl magic");
        }
        mmio_write_named("SCRATCH", 0x08, &SYS_REG_SCRATCH, 0xA5A55A5Au);
        if (SYS_REG_SCRATCH != 0xA5A55A5Au) {
            return fail("FAIL: SysCtrl scratch");
        }
        if (SYS_REG_SYS_HZ != 100000000u || SYS_REG_PCLK_HZ != 50000000u) {
            return fail("FAIL: clock frequencies");
        }
        log_line("PASS: SysCtrl");
    }

    log_line("");
    log_line("[test 1] AHB SRAM");
    {
        MMIO32(SRAM_BASE + 0x00) = 0x11111111u;
        MMIO32(SRAM_BASE + 0x04) = 0x22222222u;
        MMIO32(SRAM_BASE + 0xFFC) = 0xDEADBEEFu;
        if (MMIO32(SRAM_BASE + 0x00) != 0x11111111u ||
            MMIO32(SRAM_BASE + 0x04) != 0x22222222u ||
            MMIO32(SRAM_BASE + 0xFFC) != 0xDEADBEEFu) {
            return fail("FAIL: SRAM");
        }
        log_line("PASS: SRAM");
    }

    log_line("");
    log_line("[test 2] APB GPIO + edge IRQ status");
    {
        if (GPIO_REG_ID != GPIO_ID_VALUE) {
            return fail("FAIL: GPIO id");
        }
        GPIO_REG_DIR = 0xFFu;
        GPIO_REG_INTPOL = 0xFFu;
        GPIO_REG_INTTYPE = 0xFFu;
        GPIO_REG_INTEN = 0x01u;
        GPIO_REG_SET = 0x01u;
        if (GPIO_REG_DATA != 0x01u) {
            return fail("FAIL: GPIO data");
        }
        if ((GPIO_REG_INTSTAT & 0x01u) == 0) {
            return fail("FAIL: GPIO intstat");
        }
        GPIO_REG_INTCLR = 0x01u;
        log_line("PASS: GPIO");
    }

    log_line("");
    log_line("[test 3] APB PL011 UART loopback");
    {
        UART_IBRD = 16;
        UART_FBRD = 0;
        UART_LCRH = UART_LCRH_FEN;
        UART_CR = UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE | UART_CR_LBE;
        UART_DR = (uint32_t)'S';
        UART_DR = (uint32_t)'C';
        if ((UART_FR & UART_FR_RXFE) != 0) {
            return fail("FAIL: UART RX empty");
        }
        if ((UART_DR & 0xFFu) != (uint32_t)'S') {
            return fail("FAIL: UART loopback S");
        }
        if ((UART_DR & 0xFFu) != (uint32_t)'C') {
            return fail("FAIL: UART loopback C");
        }
        log_line("PASS: UART loopback");
    }

    log_line("");
    log_line("[test 4] UART RX IRQ via NVIC");
    {
        g_uart_irqs = 0;
        UART_ICR = UART_INT_RX;
        UART_IMSC = UART_INT_RX;
        NVIC_ISER0 = (1u << PL_IRQ_UART);
        irq_on();
        UART_DR = (uint32_t)'I';
        /* One MMIO access lets QEMU apply irq_bits, then handler runs. */
        (void)UART_FR;
        irq_off();
        UART_IMSC = 0;
        if (g_uart_irqs == 0) {
            return fail("FAIL: UART IRQ not taken");
        }
        log_line("PASS: UART IRQ");
    }

    log_line("");
    log_line("[test 5] APB watchdog");
    {
        WDT_LOCK = WDT_UNLOCK;
        WDT_LOAD = 8;
        WDT_INTCLR = 1;
        WDT_CONTROL = WDT_CTRL_INTEN;
        if (log_wait_bit(&WDT_RIS, 1u, 1000000u, "WDT_RIS") != 0) {
            return fail("FAIL: WDT timeout");
        }
        WDT_INTCLR = 1;
        WDT_CONTROL = 0;
        WDT_LOCK = 0;
        log_line("PASS: WDT");
    }

    log_line("");
    log_line("[test 6] Timer compare / overflow / disable");
    mmio_write_named("CTRL", 0x00, &TIMER_REG_CTRL, 0);
    mmio_write_named("VALUE", 0x04, &TIMER_REG_VALUE, 0);
    mmio_write_named("CMP", 0x08, &TIMER_REG_CMP, 0);
    mmio_write_named("INTR", 0x0C, &TIMER_REG_INTR, 0);
    mmio_write_named("CMP", 0x08, &TIMER_REG_CMP, 20);
    mmio_write_named("CTRL", 0x00, &TIMER_REG_CTRL,
                     TIMER_CTRL_ENABLE | TIMER_CTRL_CMP_EN | TIMER_CTRL_OV_EN);
    if (log_wait_bit(&TIMER_REG_INTR, TIMER_INTR_CMP, 1000000u, "TIMER_INTR_CMP") != 0) {
        return fail("FAIL: compare timeout");
    }
    log_line("PASS: compare status set");
    mmio_write_named("INTR", 0x0C, &TIMER_REG_INTR, TIMER_REG_INTR & ~TIMER_INTR_CMP);
    mmio_write_named("VALUE", 0x04, &TIMER_REG_VALUE, 250);
    if (log_wait_bit(&TIMER_REG_INTR, TIMER_INTR_OV, 1000000u, "TIMER_INTR_OV") != 0) {
        return fail("FAIL: overflow timeout");
    }
    log_line("PASS: overflow status set");
    mmio_write_named("CTRL", 0x00, &TIMER_REG_CTRL, 0);
    {
        uint32_t before = TIMER_REG_VALUE;
        for (volatile int i = 0; i < 1000; i++) {
        }
        if (before != TIMER_REG_VALUE) {
            return fail("FAIL: timer still running");
        }
    }
    log_line("PASS: timer stopped when disabled");

    log_line("");
    log_line("========================================");
    log_line("  ALL TESTS COMPLETED");
    log_line("========================================");
    sh_exit(0);
    return 0;
}

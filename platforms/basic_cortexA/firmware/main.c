#include <stdint.h>
#include "timer_regs.h"
#include "uart_console.h"
#include "semihost.h"
#include "log.h"

static void exit_app(int code)
{
    if (code) {
        log_line("FAIL: exiting with error");
    } else {
        log_line("[boot] Clean shutdown via semihosting");
    }
    sh_exit(code);
    while (1) {
    }
}

static void mmio_write_ctrl(uint32_t v)
{
    log_mmio_write("CTRL", 0x00, v);
    TIMER_REG_CTRL = v;
}

static void mmio_write_value(uint32_t v)
{
    log_mmio_write("VALUE", 0x04, v);
    TIMER_REG_VALUE = v;
}

static void mmio_write_cmp(uint32_t v)
{
    log_mmio_write("CMP", 0x08, v);
    TIMER_REG_CMP = v;
}

static void mmio_write_intr(uint32_t v)
{
    log_mmio_write("INTR", 0x0C, v);
    TIMER_REG_INTR = v;
}

static uint32_t mmio_read_intr(void)
{
    uint32_t v = TIMER_REG_INTR;
    log_mmio_read("INTR", 0x0C, v);
    return v;
}

int main(void)
{
    log_line("========================================");
    log_line("  basic_cortexA — Timer cosim firmware");
    log_line("========================================");
    log_line("[boot] CPU: Cortex-A9   machine: systemc-ps");
    log_line("[boot] Console: PL011 UART @ 0x10009000");
    log_line("[boot] PL window -> SystemC Timer (remote-mmio)");
    uart_puts("[boot] TIMER base ");
    log_put_hex32(TIMER_BASE);
    uart_puts("\n");
    log_line("[boot] Map: CTRL +0x00  VALUE +0x04  CMP +0x08  INTR +0x0C");
    log_line("");

    log_line("[init] Phase 0 — reset timer registers");
    mmio_write_ctrl(0);
    mmio_write_value(0);
    mmio_write_cmp(0);
    mmio_write_intr(0);
  {
    uint32_t v = TIMER_REG_VALUE;
    log_mmio_read("VALUE", 0x04, v);
  }

    log_line("");
    log_line("[test 1] Compare match — program CMP=20, enable timer");
    mmio_write_cmp(20);
    mmio_write_ctrl(TIMER_CTRL_ENABLE | TIMER_CTRL_CMP_EN | TIMER_CTRL_OV_EN);

    if (log_wait_bit(&TIMER_REG_INTR, TIMER_INTR_CMP, 1000000u, "TIMER_INTR_CMP") != 0) {
        log_line("FAIL: compare status timeout");
        exit_app(1);
    }
    log_line("PASS: compare status set");
    mmio_write_intr(mmio_read_intr() & ~TIMER_INTR_CMP);

    log_line("");
    log_line("[test 2] Overflow — preset VALUE=250, wait for OV bit");
    mmio_write_value(250);
    if (log_wait_bit(&TIMER_REG_INTR, TIMER_INTR_OV, 1000000u, "TIMER_INTR_OV") != 0) {
        log_line("FAIL: overflow status timeout");
        exit_app(1);
    }
    log_line("PASS: overflow status set");

    log_line("");
    log_line("[test 3] Disable — CTRL=0, VALUE must not change");
    mmio_write_ctrl(0);
    uint32_t before = TIMER_REG_VALUE;
    log_mmio_read("VALUE", 0x04, before);
    for (volatile int i = 0; i < 1000; i++) {
    }
    uint32_t after = TIMER_REG_VALUE;
    log_mmio_read("VALUE", 0x04, after);
    if (before != after) {
        log_line("FAIL: timer still running after disable");
        exit_app(1);
    }
    log_line("PASS: timer stopped when disabled");

    log_line("");
    log_line("========================================");
    log_line("  ALL TESTS COMPLETED");
    log_line("========================================");
    exit_app(0);
    return 0;
}

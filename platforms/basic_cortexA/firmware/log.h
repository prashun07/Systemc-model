#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include "uart_console.h"

static inline void log_put_hex32(uint32_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        char c[2] = { hex[(v >> shift) & 0xFu], '\0' };
        uart_putc(c[0]);
    }
}

static inline void log_line(const char *msg)
{
    uart_puts(msg);
    uart_puts("\n");
}

static inline void log_mmio_write(const char *reg, uint32_t offset, uint32_t value)
{
    uart_puts("[mmio] WRITE ");
    uart_puts(reg);
    uart_puts(" @+");
    log_put_hex32(offset);
    uart_puts(" = ");
    log_put_hex32(value);
    uart_puts("\n");
}

static inline void log_mmio_read(const char *reg, uint32_t offset, uint32_t value)
{
    uart_puts("[mmio] READ  ");
    uart_puts(reg);
    uart_puts(" @+");
    log_put_hex32(offset);
    uart_puts(" -> ");
    log_put_hex32(value);
    uart_puts("\n");
}

static inline int log_wait_bit(volatile uint32_t *reg, uint32_t mask,
                               uint32_t spins, const char *label)
{
    uint32_t polls = 0;
    uart_puts("[poll] waiting for ");
    uart_puts(label);
    uart_puts(" (mask ");
    log_put_hex32(mask);
    uart_puts(")...\n");

    while (spins--) {
        polls++;
        uint32_t v = *reg;
        if ((v & mask) != 0) {
            uart_puts("[poll] ");
            uart_puts(label);
            uart_puts(" set after ");
            log_put_hex32(polls);
            uart_puts(" reads, INTR=");
            log_put_hex32(v);
            uart_puts("\n");
            return 0;
        }
        if ((polls % 200000u) == 0u) {
            uart_puts("[poll] still waiting (");
            log_put_hex32(polls);
            uart_puts(" reads, INTR=");
            log_put_hex32(v);
            uart_puts(")\n");
        }
    }

    uart_puts("[poll] TIMEOUT ");
    uart_puts(label);
    uart_puts(" after ");
    log_put_hex32(polls);
    uart_puts(" reads, INTR=");
    log_put_hex32(*reg);
    uart_puts("\n");
    return -1;
}

#endif /* LOG_H */

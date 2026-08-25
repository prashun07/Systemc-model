#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include "semihost.h"

static inline void log_put_hex32(uint32_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    sh_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        char c[2] = { hex[(v >> shift) & 0xFu], '\0' };
        sh_puts(c);
    }
}

static inline void log_line(const char *msg)
{
    sh_puts(msg);
    sh_puts("\n");
}

static inline void log_mmio_write(const char *reg, uint32_t offset, uint32_t value)
{
    sh_puts("[mmio] WRITE ");
    sh_puts(reg);
    sh_puts(" @+");
    log_put_hex32(offset);
    sh_puts(" = ");
    log_put_hex32(value);
    sh_puts("\n");
}

static inline void log_mmio_read(const char *reg, uint32_t offset, uint32_t value)
{
    sh_puts("[mmio] READ  ");
    sh_puts(reg);
    sh_puts(" @+");
    log_put_hex32(offset);
    sh_puts(" -> ");
    log_put_hex32(value);
    sh_puts("\n");
}

static inline int log_wait_bit(volatile uint32_t *reg, uint32_t mask,
                               uint32_t spins, const char *label)
{
    uint32_t polls = 0;
    sh_puts("[poll] waiting for ");
    sh_puts(label);
    sh_puts(" (mask ");
    log_put_hex32(mask);
    sh_puts(")...\n");

    while (spins--) {
        polls++;
        uint32_t v = *reg;
        if ((v & mask) != 0) {
            sh_puts("[poll] ");
            sh_puts(label);
            sh_puts(" set after ");
            log_put_hex32(polls);
            sh_puts(" reads, INTR=");
            log_put_hex32(v);
            sh_puts("\n");
            return 0;
        }
        if ((polls % 200000u) == 0u) {
            sh_puts("[poll] still waiting (");
            log_put_hex32(polls);
            sh_puts(" reads, INTR=");
            log_put_hex32(v);
            sh_puts(")\n");
        }
    }

    sh_puts("[poll] TIMEOUT ");
    sh_puts(label);
    sh_puts(" after ");
    log_put_hex32(polls);
    sh_puts(" reads, INTR=");
    log_put_hex32(*reg);
    sh_puts("\n");
    return -1;
}

#endif /* LOG_H */

#ifndef UART_CONSOLE_H
#define UART_CONSOLE_H

#include <stdint.h>

/* PL011 on systemc-ps (see qemu_soc/include/soc_memory_map.h) */
#define UART0_BASE  0x10009000u
#define UARTDR      (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UARTFR      (*(volatile uint32_t *)(UART0_BASE + 0x18))
#define UARTFR_TXFF (1u << 5)

static inline void uart_putc(char c)
{
    while (UARTFR & UARTFR_TXFF) {
    }
    UARTDR = (uint32_t)(unsigned char)c;
}

static inline void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

#endif /* UART_CONSOLE_H */

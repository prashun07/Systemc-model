/* Cortex-A9 startup for systemc-ps (loaded at DDR base). */

  .cpu cortex-a9
  .section .text.boot, "ax"
  .global _start

_start:
  /* Set stack near top of first 1 MiB of DDR */
  ldr sp, =0x600FF000
  bl main
1:
  b 1b

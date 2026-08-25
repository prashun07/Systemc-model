ENTRY(_start)

MEMORY
{
  RAM (rwx) : ORIGIN = 0x60000000, LENGTH = 256M
}

SECTIONS
{
  .text.boot : {
    KEEP(*(.text.boot))
  } > RAM

  .text : {
    *(.text*)
    *(.rodata*)
  } > RAM

  .data : {
    *(.data*)
  } > RAM

  .bss : {
    *(.bss*)
    *(COMMON)
  } > RAM
}

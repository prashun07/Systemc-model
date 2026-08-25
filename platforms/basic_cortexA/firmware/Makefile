ROOT    ?= ../../..
CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

CFLAGS  := -mcpu=cortex-a9 -O2 -ffreestanding -fno-builtin \
           -Wall -Wextra -I. -I$(ROOT)/include
LDFLAGS := -T linker.ld -nostdlib -Wl,--gc-sections

OBJS := startup.o main.o

.PHONY: all clean

all: timer_ps.elf timer_ps.bin
	$(SIZE) timer_ps.elf

timer_ps.elf: $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

timer_ps.bin: timer_ps.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) timer_ps.elf timer_ps.bin

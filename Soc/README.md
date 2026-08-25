# Cortex-M PL SoC (SystemC / TLM-2.0 LT)

Assembled in `soc/cortexm_pl.h`. Blocks live in role directories at the repo root.

```text
processor/          QEMU Cortex-M3 / Cortex-A9 machines
transactor/         CosimServer (socket → TLM)
interconnect/       AHB / APB decoders
bridges/            remote-mmio, AHB-APB, pin transactor
peripherals/        UART, GPIO, WDT, SRAM, SysCtrl, Timer
clocks/             sys_clk / pclk / reset
include/            memory map, TLM helpers, SCM1 protocol
soc/                top-level SystemC SoC wiring
platforms/          firmware + sc_main
```

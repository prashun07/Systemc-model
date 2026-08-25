# Platforms

A **platform** is a complete cosim target: SystemC wiring (`cosim_main.cpp`), guest **firmware**, and a `.env` file.

SystemC IP and QEMU machines live in top-level role directories (`processor/`, `transactor/`, `interconnect/`, `bridges/`, `peripherals/`). `platforms/` only connects them.

| Piece | Location | Role |
|-------|----------|------|
| Peripherals | `peripherals/` | SystemC IP |
| Transactor / fabric | `transactor/`, `interconnect/`, `bridges/` | Socket + buses |
| Processor | `processor/` | QEMU machines |
| Platform | `platforms/basic_cortexM/`, … | `cosim_main` + `firmware/` + `.env` |

## Reference platforms

| Platform | CPU / machine | Description |
|----------|---------------|-------------|
| `basic_cortexM` | Cortex-M3 / `systemc-soc` | AHB/APB PL SoC @ `0x40000000` (UART, GPIO, WDT, SRAM, Timer) |
| `basic_cortexA` | Cortex-A9 / `systemc-ps` | DDR + UART + GIC + PL @ `0xF0000000`; Timer demo firmware |

## Quick start

From `systemc_model/`:

```bash
./scripts/build_qemu.sh      # once
./scripts/run_platform.sh basic_cortexM
./scripts/run_platform.sh basic_cortexA
```

Or list all platforms:

```bash
./scripts/run_platform.sh list
```

## Add a new model

1. **Create your IP** under `peripherals/` (e.g. `peripherals/MyPeriph/`). Validate with a standalone testbench.

2. **Copy a reference platform:**
   ```bash
   cp -r platforms/basic_cortexM platforms/my_periph_m3
   ```

3. **Edit `cosim_main.cpp`** — instantiate your module and include your header.

4. **Replace `firmware/`** — guest program and register headers for your peripheral.

5. **Create `platforms/my_periph_m3.env`:**
   ```bash
   PLATFORM_DIR=platforms/my_periph_m3
   MODEL_DIR=peripherals/MyPeriph
   QEMU_MACHINE=systemc-soc
   QEMU_CPU=cortex-m3
   SYSTEMC_PL_BASE=0x40000000
   FIRMWARE_DIR=platforms/my_periph_m3/firmware
   FIRMWARE_ELF=my_periph_fw.elf
   QEMU_EXTRA_ARGS=""
   QEMU_MONITOR_ARGS="-monitor none"
   ```

6. **Run:** `./scripts/run_platform.sh my_periph_m3`

No QEMU machine changes are required for pin-level models that follow `bridges/peripheral_if.h`.

## Platform env variables

| Variable | Meaning |
|----------|---------|
| `PLATFORM_DIR` | Directory with `cosim_main.cpp`, `Makefile`, and usually `firmware/` |
| `MODEL_DIR` | User SystemC model (relative to repo root) |
| `QEMU_MACHINE` | `-M` machine (`systemc-soc` or `systemc-ps`) |
| `QEMU_CPU` | `-cpu` argument |
| `SYSTEMC_PL_BASE` | PL window base (must match QEMU machine) |
| `FIRMWARE_DIR` | Firmware Makefile directory |
| `FIRMWARE_ELF` | Kernel image for QEMU `-kernel` |
| `QEMU_EXTRA_ARGS` | Optional extra QEMU flags |
| `QEMU_MONITOR_ARGS` | Monitor flags (default `-monitor none`) |

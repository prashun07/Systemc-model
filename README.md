# SystemC virtual platform — QEMU cosimulation

Cosimulate **QEMU (system mode)** with **SystemC peripheral models** in a separate process. QEMU provides the CPU and memory map; your IP (e.g. Timer) runs in SystemC. QEMU only contains a generic `remote-mmio` socket bridge — no peripheral logic is compiled into QEMU.

---

## Repository layout

```text
systemc_model/
  scripts/           setup_host.sh, build_qemu.sh, run_platform.sh, config.sh
  platforms/         cosim harness + firmware + *.env per target
  Timer/             user SystemC IP (no QEMU dependency)
  qemu_soc/          generic bridge, TLM wrapper, QEMU machine patches
  Setup.md           install and first-run steps
```

| Piece | Location | Role |
|-------|----------|------|
| User model | `Timer/` | SystemC peripheral IP |
| Cosim bridge | `qemu_soc/` | Socket, TLM, `systemc-soc` / `systemc-ps` machines |
| Platform | `platforms/basic_cortexM/`, `basic_cortexA/` | `cosim_main.cpp` + `firmware/` + `.env` |

Models and `qemu_soc/` are **independent**. A platform under `platforms/` wires them at run time.

---

## Reference platforms

| Platform | CPU / QEMU machine | PL base | Console |
|----------|-------------------|---------|---------|
| `basic_cortexM` | Cortex-M3 / `systemc-soc` | `0x40000000` | Semihosting |
| `basic_cortexA` | Cortex-A9 / `systemc-ps` | `0xF0000000` | PL011 UART |

### Quick start

**New machine** — install deps, SystemC, env, and QEMU in one step:

```bash
./scripts/setup_host.sh --full
source ~/.zshrc   # or ~/.bashrc
```

Then from `systemc_model/`:

```bash
./scripts/run_platform.sh list       # show platforms
./scripts/run_platform.sh basic_cortexM
./scripts/run_platform.sh basic_cortexA
```

See [`Setup.md`](Setup.md) for options (`--deps-only`, `--verify`, etc.) and manual install steps.

Use **custom QEMU** from `~/qemu-systemc` (not Homebrew). Homebrew `qemu-system-arm` lacks `systemc-soc` / `systemc-ps`.

---

## Two-process cosim (Cortex-M3 example)

| Process | Binary | Role |
|---------|--------|------|
| **SystemC** | `platforms/basic_cortexM/cosim_platform` | Timer model, TLM bus, socket server |
| **QEMU** | `qemu-system-arm -M systemc-soc` | Cortex-M3, Flash, SRAM, NVIC, MMIO bridge |

```text
 ┌─────────────────────────────────────────────────────────────────────────┐
 │                         HOST (macOS / Linux)                            │
 │                                                                         │
 │  QEMU (systemc-soc)                     SystemC (cosim_platform)        │
 │  ┌────────────────────────┐            ┌────────────────────────┐     │
 │  │ Cortex-M3              │            │ sc_clock (10 ns)       │     │
 │  │ Flash / SRAM / NVIC    │            │ CosimServer (TLM)      │     │
 │  │ remote-mmio @0x40000000├─Unix socket► TlmAddressMap           │     │
 │  │                        │            │ TlmPinBridge → Timer   │     │
 │  └────────────────────────┘            └────────────────────────┘     │
 │  Guest: platforms/basic_cortexM/firmware/timer_fw.elf                  │
 └─────────────────────────────────────────────────────────────────────────┘
```

**Data path:** guest MMIO → `remote-mmio` → socket → `CosimServer` → `TlmAddressMap` → `TlmPinBridge` → `Timer/`

---

## Expected output (verbose firmware logs)

Firmware prints boot banner, each MMIO access, poll progress, and test phases. SystemC prints platform topology and reset timing.

```text
==> Platform: basic_cortexM
==> Data path: QEMU guest MMIO -> remote-mmio -> /tmp/systemc_cosim.sock
               -> CosimServer -> TlmAddressMap -> TlmPinBridge -> .../Timer

[platform] === basic_cortexM cosim platform ===
[platform] topology: CosimServer -> TlmAddressMap -> TlmPinBridge -> Timer
[cosim] listening on /tmp/systemc_cosim.sock
[platform] basic_cortexM ready; PL @ 0x40000000 ...

========================================
  basic_cortexM — Timer cosim firmware
========================================
[boot] CPU: Cortex-M3  machine: systemc-soc
[boot] TIMER base 0x40000000
[init] Phase 0 — reset timer registers
[mmio] WRITE CTRL @+0x00000000 = 0x00000000
[cosim] QEMU connected @9900 us
...
[test 1] Compare match — program CMP=20, enable timer
[mmio] WRITE CMP @+0x00000008 = 0x00000014
[poll] TIMER_INTR_CMP set after 0x00000001 reads, INTR=0x00000006
PASS: compare status set
...
  ALL TESTS COMPLETED
```

On **basic_cortexA**, the same test flow appears over **PL011 UART** (`-nographic` → terminal).

---

## What QEMU owns vs what SystemC owns

| Component | QEMU | SystemC |
|-----------|------|---------|
| CPU, memory, interrupt controller | Yes | — |
| PL MMIO window | Address decode + socket bridge only | Real peripheral behavior |
| `sc_clock` / cycle-accurate timer | No | Yes |

Guest firmware uses absolute PL addresses (e.g. `0x40000000`). QEMU forwards **window offsets** (`0x00`–`0x0C`) to SystemC.

---

## Memory maps

### Cortex-M3 (`systemc-soc`)

| Region | Base | Role |
|--------|------|------|
| Flash | `0x00000000` | `timer_fw.elf` |
| SRAM | `0x20000000` | Stack / data |
| Timer (PL) | `0x40000000` | → SystemC via `remote-mmio` |

### Cortex-A9 (`systemc-ps`)

| Region | Base | Role |
|--------|------|------|
| DDR | `0x60000000` | Guest RAM |
| PL011 UART | `0x10009000` | Console |
| Timer (PL) | `0xF0000000` | → SystemC via `remote-mmio` |

See `qemu_soc/include/soc_memory_map.h`.

### Timer registers (firmware ↔ model)

| Offset | Register | Meaning |
|--------|----------|---------|
| `+0x00` | CTRL | ENABLE, CMP_EN, OV_EN |
| `+0x04` | VALUE | Counter |
| `+0x08` | CMP | Compare threshold |
| `+0x0C` | INTR | Compare / overflow status |

Headers: `platforms/*/firmware/timer_regs.h` and `Timer/timer.h`.

---

## How the two sides connect

### Unix socket + SCM1 protocol

Default socket: `/tmp/systemc_cosim.sock` (`SYSTEMC_COSIM_SOCKET`).

```text
CosimRequest  { magic='SCM1', op=READ|WRITE, addr=offset, data }
CosimResponse { magic='SCM1', status=OK, data }
```

Each guest MMIO access blocks until SystemC responds.

### SystemC path

```text
CosimServer → TlmAddressMap → TlmPinBridge → Timer
```

Wiring in `platforms/basic_cortexM/cosim_main.cpp` (same topology for `basic_cortexA`).

### QEMU path

`qemu/remote_mmio.c` on guest load/store → socket RPC. Machines: `qemu/systemc_soc.c`, `qemu/systemc_ps.c`.

---

## Startup sequence (`run_platform.sh`)

```text
1. Load platforms/<name>.env
2. make -C <platform>/firmware
3. make -C <platform>  → cosim_platform
4. Start cosim_platform (listen on socket)
5. Start qemu-system-arm with -kernel <firmware.elf>
6. On exit: stop SystemC, remove socket
```

---

## Key source files

| Layer | Path |
|-------|------|
| Run | `scripts/run_platform.sh`, `scripts/config.sh` |
| Platform configs | `platforms/basic_cortexM.env`, `platforms/basic_cortexA.env` |
| Platform top | `platforms/*/cosim_main.cpp` |
| Firmware + logs | `platforms/*/firmware/main.c`, `log.h` |
| User model | `Timer/timer.h` |
| Socket server | `qemu_soc/wrapper/cosim_server.cpp` |
| TLM fabric | `qemu_soc/wrapper/tlm_address_map.h`, `tlm_pin_bridge.h` |
| QEMU bridge | `qemu_soc/qemu/remote_mmio.c` |
| QEMU machines | `qemu_soc/qemu/systemc_soc.c`, `systemc_ps.c` |
| Protocol | `qemu_soc/protocol/cosim_protocol.h` |

---

## Adding a new model

1. Create IP under e.g. `MyPeriph/` (standalone testbench first).
2. Copy `platforms/basic_cortexM` or `platforms/_template/`.
3. Edit `cosim_main.cpp` and `firmware/`.
4. Add `platforms/my_model_m3.env`.
5. Run `./scripts/run_platform.sh my_model_m3`.

No `qemu_soc/` changes needed for pin-level models following `qemu_soc/wrapper/peripheral_if.h`.

Details: [`platforms/README.md`](platforms/README.md).

---

## Design rules

1. **No peripheral logic in QEMU** — only `remote-mmio`.
2. **Pin-level models** — `TlmPinBridge` adapts TLM to `read_en`/`write_en`.
3. **QEMU sends window offsets** — `TlmAddressMap` decodes from base `0`.
4. **Functional cosim** — not cycle-locked to QEMU TCG.
5. **IRQ back-channel** — NVIC/GIC lines wired; firmware polls `INTR` today.

---

## Further reading

| Document | Content |
|----------|---------|
| [`Setup.md`](Setup.md) | SystemC/QEMU install, env vars, first run |
| [`platforms/README.md`](platforms/README.md) | Platform `.env` variables, new model checklist |
| [`Timer/Timer.md`](Timer/Timer.md) | Timer model + SystemC interview Q&A |
| [`Timer/README.md`](Timer/README.md) | Full Timer architecture and cosim call chains |
| [`platforms/basic_cortexM/firmware/startup.md`](platforms/basic_cortexM/firmware/startup.md) | Cortex-M boot / vector table |

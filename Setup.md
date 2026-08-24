# SystemC Model — Setup Guide

Install and run QEMU ↔ SystemC cosimulation on a **new machine** (macOS or Linux). Two paths:

| Path | When to use |
|------|-------------|
| **[Automated (recommended)](#automated-setup-script)** | Fresh clone; let `setup_host.sh` install packages, SystemC, env, and optionally QEMU |
| **[Manual](#manual-setup)** | You want full control, custom paths, or a distro not covered by the script |

---

## What you are setting up

| Component | Default location | Purpose |
|-----------|------------------|---------|
| **SystemC** | `~/systemc/install` | SystemC library for the Timer model and cosim platform |
| **Custom QEMU** | `~/qemu-systemc` | `qemu-system-arm` with `systemc-soc` / `systemc-ps` machines + `remote-mmio` bridge |
| **ARM toolchain** | `arm-none-eabi-gcc` (PATH) | Build baremetal guest firmware |
| **Cosim socket** | `/tmp/systemc_cosim.sock` | Unix socket between QEMU and SystemC |

**Important:** Homebrew or system QEMU **cannot** be used for cosim. Stock builds lack the `systemc-soc` / `systemc-ps` machines and the `remote-mmio` socket bridge. You must build QEMU with `./scripts/build_qemu.sh`.

---

## Repository layout

```text
systemc_model/
  scripts/
    setup_host.sh       # machine-aware one-time host setup
    setup_lib.sh        # helpers (sourced by setup_host.sh)
    build_qemu.sh       # clone QEMU v10.0.0, apply patches, build
    run_platform.sh     # build firmware + cosim, launch both processes
    config.sh           # platform paths and *.env loader
  platforms/
    basic_cortexM/      # Cortex-M3 cosim + Timer firmware
    basic_cortexA/      # Cortex-A9 cosim + Timer firmware
    basic_cortexM.env   # platform config (machine, PL base, firmware)
    basic_cortexA.env
  Timer/                # SystemC IP (no QEMU dependency)
  qemu_soc/             # generic bridge, TLM wrapper, QEMU machine sources
    qemu/               # systemc_soc.c, systemc_ps.c, remote_mmio.c
    wrapper/            # CosimServer, TLM bus, pin bridge
    scripts/            # forwards to top-level scripts/
```

`Timer/` and `qemu_soc/` are **independent**. A platform under `platforms/` wires them together at run time.

### Reference platforms

| Platform | CPU | QEMU machine | PL base | Console |
|----------|-----|--------------|---------|---------|
| `basic_cortexM` | Cortex-M3 | `systemc-soc` | `0x40000000` | Semihosting |
| `basic_cortexA` | Cortex-A9 | `systemc-ps` | `0xF0000000` | PL011 UART |

---

## Environment variables

After setup, these should be set in your shell (`~/.zshrc` or `~/.bashrc`):

| Variable | Default | Description |
|----------|---------|-------------|
| `SYSTEMC_HOME` | `~/systemc/install` | SystemC install prefix |
| `SYSTEMC_INCLUDE` | `$SYSTEMC_HOME/include` | Headers for cosim build |
| `SYSTEMC_LIBDIR` | `$SYSTEMC_HOME/lib` | Shared library path |
| `QEMU_PREFIX` | `~/qemu-systemc` | Custom QEMU install prefix |
| `PATH` | `$QEMU_PREFIX/bin:…` | So `qemu-system-arm` resolves to custom build |
| `DYLD_LIBRARY_PATH` (macOS) or `LD_LIBRARY_PATH` (Linux) | `$SYSTEMC_LIBDIR:…` | Runtime loader for `libsystemc` |

Optional overrides (used by `run_platform.sh` / `build_qemu.sh`):

| Variable | Default | Description |
|----------|---------|-------------|
| `SYSTEMC_COSIM_SOCKET` | `/tmp/systemc_cosim.sock` | Unix socket path |
| `QEMU` | `$QEMU_PREFIX/bin/qemu-system-arm` | QEMU binary |
| `QEMU_SRC_DIR` | `~/qemu-systemc-src` | QEMU source checkout |
| `QEMU_BUILD_DIR` | `~/qemu-systemc-build` | QEMU build directory |
| `QEMU_VERSION` | `v10.0.0` | QEMU tag to build |
| `JOBS` | CPU count | Parallel build jobs for QEMU |

The setup script writes a marked block into your shell rc file:

```text
# >>> systemc_model cosim env >>>
…
# <<< systemc_model cosim env <<<
```

Re-running `setup_host.sh` skips writing if that block already exists.

---

## Automated setup (script)

### Prerequisites

- **macOS:** [Homebrew](https://brew.sh) installed
- **Linux:** `sudo` access for apt or dnf
- **Both:** `git`, network access to download SystemC and QEMU sources

### One-command full setup

From the repo root (`systemc_model/`):

```bash
git clone <your-repo-url> systemc_model
cd systemc_model
chmod +x scripts/*.sh
./scripts/setup_host.sh --full
```

`--full` does everything in order:

1. Detect OS (`macOS` or `Linux`) and CPU count
2. Install host packages
3. Download and build **SystemC 2.3.4** → `~/systemc/install`
4. Append environment block to `~/.zshrc` (zsh) or `~/.bashrc` (bash)
5. Build **custom QEMU** via `./scripts/build_qemu.sh` (~15–30 min first time)
6. Verify required tools and SystemC install

### Reload shell and run

```bash
source ~/.zshrc    # or: source ~/.bashrc
./scripts/setup_host.sh --verify
./scripts/run_platform.sh list
./scripts/run_platform.sh basic_cortexM
./scripts/run_platform.sh basic_cortexA
```

### Script options

| Command | What it does |
|---------|----------------|
| `./scripts/setup_host.sh` | Deps + SystemC + env block (no QEMU) |
| `./scripts/setup_host.sh --full` | Above + QEMU build |
| `./scripts/setup_host.sh --deps-only` | OS packages only |
| `./scripts/setup_host.sh --systemc-only` | Build SystemC only (skip OS packages) |
| `./scripts/setup_host.sh --qemu-only` | Run `build_qemu.sh` only |
| `./scripts/setup_host.sh --verify` | Check tools and SystemC; exit 0 if OK |
| `./scripts/setup_host.sh --help` | Show usage |

### What the script installs per OS

**macOS (Homebrew):**

```text
cmake git ninja pkg-config glib pixman arm-none-eabi-gcc python3
```

**Ubuntu / Debian (apt):**

```text
build-essential cmake git ninja-build pkg-config
libglib2.0-dev libpixman-1-dev gcc-arm-none-eabi python3 curl ca-certificates
```

**Fedora / RHEL (dnf):**

```text
gcc gcc-c++ cmake git ninja-build pkgconfig
glib2-devel pixman-devel arm-none-eabi-gcc-cs python3 curl
```

### Custom paths (script)

```bash
SYSTEMC_VERSION=2.3.4 \
SYSTEMC_INSTALL=$HOME/opt/systemc \
SYSTEMC_SRC_ROOT=$HOME/src/systemc \
./scripts/setup_host.sh --full
```

QEMU paths are controlled by `build_qemu.sh` (`QEMU_PREFIX`, `QEMU_SRC_DIR`, `QEMU_BUILD_DIR`).

### Verify QEMU after script setup

```bash
$QEMU_PREFIX/bin/qemu-system-arm -machine help | grep systemc
```

Expected lines include `systemc-soc` and `systemc-ps`.

---

## Manual setup

Follow these steps if you prefer not to use `setup_host.sh`, or your distro is unsupported.

### Step 0 — Clone the repository

```bash
git clone <your-repo-url> systemc_model
cd systemc_model
chmod +x scripts/*.sh
```

### Step 1 — Install host packages

#### macOS

Install [Homebrew](https://brew.sh), then:

```bash
brew update
brew install cmake git ninja pkg-config glib pixman arm-none-eabi-gcc python3
```

Do **not** rely on `brew install qemu` for cosim — you need the custom build in Step 4.

#### Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake git ninja-build pkg-config \
  libglib2.0-dev libpixman-1-dev gcc-arm-none-eabi python3 \
  curl ca-certificates
```

#### Fedora / RHEL

```bash
sudo dnf install -y gcc gcc-c++ cmake git ninja-build pkgconfig \
  glib2-devel pixman-devel arm-none-eabi-gcc-cs python3 curl
```

#### Verify tools

```bash
cmake --version
ninja --version
pkg-config --version
arm-none-eabi-gcc --version
python3 --version
```

### Step 2 — Build SystemC from source

SystemC is **not** available as a standard OS package for this project; build Accellera 2.3.4:

```bash
mkdir -p ~/systemc && cd ~/systemc
curl -fsSL -o systemc-2.3.4.tar.gz \
  https://github.com/accellera-official/systemc/archive/refs/tags/2.3.4.tar.gz
tar -xzf systemc-2.3.4.tar.gz
cmake -S systemc-2.3.4 -B systemc-2.3.4/build \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$HOME/systemc/install
cmake --build systemc-2.3.4/build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
cmake --install systemc-2.3.4/build
```

Confirm:

```bash
test -f ~/systemc/install/include/systemc.h && echo "SystemC OK"
```

### Step 3 — Set environment variables

Add to `~/.zshrc` (macOS default) or `~/.bashrc` (Linux):

```bash
export SYSTEMC_HOME=$HOME/systemc/install
export SYSTEMC_INCLUDE=$SYSTEMC_HOME/include
export SYSTEMC_LIBDIR=$SYSTEMC_HOME/lib
export QEMU_PREFIX=$HOME/qemu-systemc
export PATH="$QEMU_PREFIX/bin:$PATH"
```

**macOS** — also add:

```bash
export DYLD_LIBRARY_PATH="$SYSTEMC_LIBDIR:${DYLD_LIBRARY_PATH:-}"
```

**Linux** — also add:

```bash
export LD_LIBRARY_PATH="$SYSTEMC_LIBDIR:${LD_LIBRARY_PATH:-}"
```

Reload:

```bash
source ~/.zshrc    # or source ~/.bashrc
```

### Step 4 — Build custom QEMU

From `systemc_model/`:

```bash
./scripts/build_qemu.sh
```

This script:

1. Clones QEMU **v10.0.0** to `~/qemu-systemc-src` (if missing)
2. Copies machine sources from `qemu_soc/qemu/` into the QEMU tree
3. Patches Kconfig / meson.build for `REMOTE_MMIO`, `SYSTEMC_SOC`, `SYSTEMC_PS`
4. Configures `arm-softmmu` only, installs to `~/qemu-systemc`

Override paths if needed:

```bash
QEMU_PREFIX=$HOME/opt/qemu-systemc \
QEMU_SRC_DIR=$HOME/src/qemu \
QEMU_BUILD_DIR=$HOME/build/qemu-systemc \
JOBS=8 \
./scripts/build_qemu.sh
```

Verify:

```bash
$QEMU_PREFIX/bin/qemu-system-arm -machine help | grep systemc
```

You should see `systemc-soc` and `systemc-ps`.

### Step 5 (optional) — Standalone Timer testbench

Test SystemC **without QEMU**:

```bash
cd Timer
c++ -std=c++17 -I"$SYSTEMC_INCLUDE" timer_tb.cpp \
  -L"$SYSTEMC_LIBDIR" -lsystemc -o timer_sim
```

Run:

```bash
# macOS
DYLD_LIBRARY_PATH="$SYSTEMC_LIBDIR" ./timer_sim

# Linux
LD_LIBRARY_PATH="$SYSTEMC_LIBDIR" ./timer_sim
```

---

## Run cosimulation

All commands from **`systemc_model/`**.

### List platforms

```bash
./scripts/run_platform.sh list
```

### Run a platform

```bash
./scripts/run_platform.sh basic_cortexM    # Cortex-M3 + semihosting console
./scripts/run_platform.sh basic_cortexA    # Cortex-A9 + PL011 UART console
```

`run_platform.sh` automatically:

1. Loads `platforms/<name>.env`
2. Builds firmware (`make` in `platforms/<name>/firmware/`)
3. Builds SystemC cosim binary (`make` in `platforms/<name>/`)
4. Starts SystemC socket server on `/tmp/systemc_cosim.sock`
5. Starts QEMU with the matching machine and firmware ELF

### Cosim data path

```text
QEMU guest MMIO
  → remote-mmio (in QEMU)
  → Unix socket (/tmp/systemc_cosim.sock)
  → CosimServer (qemu_soc/wrapper)
  → TlmAddressMap → TlmPinBridge
  → Timer/ (or your model)
```

### Expected output (abbreviated)

```text
==> Platform: basic_cortexM
==> Data path: QEMU guest MMIO -> remote-mmio -> /tmp/systemc_cosim.sock
               -> CosimServer -> TlmAddressMap -> TlmPinBridge -> .../Timer

[platform] === basic_cortexM cosim platform ===
[cosim] listening on /tmp/systemc_cosim.sock
[platform] basic_cortexM ready; PL @ 0x40000000 ...
[cosim] QEMU connected @9900 us

========================================
  basic_cortexM — Timer cosim firmware
========================================
[boot] CPU: Cortex-M3  machine: systemc-soc
[mmio] WRITE CTRL @+0x00000000 = 0x00000000
...
PASS: compare status set
PASS: overflow status set
PASS: timer stopped when disabled
  ALL TESTS COMPLETED
```

On **basic_cortexA**, the same test flow prints over **PL011 UART** (`-nographic` routes UART to the terminal).

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| `qemu-system-arm: unsupported machine type systemc-soc` | Using Homebrew/system QEMU | Run `./scripts/build_qemu.sh`; ensure `$QEMU_PREFIX/bin` is first on `PATH` |
| `error: …/qemu-system-arm not found` | QEMU not built | `./scripts/setup_host.sh --qemu-only` or `./scripts/build_qemu.sh` |
| `libsystemc.so` / loader error at runtime | `DYLD_LIBRARY_PATH` / `LD_LIBRARY_PATH` unset | `source ~/.zshrc`; confirm `SYSTEMC_LIBDIR` is exported |
| SystemC cosim socket did not appear | SystemC binary failed to start | Run `platforms/<name>/cosim_platform` manually; check `SYSTEMC_HOME` |
| `basic_cortexA` hangs at startup with no output | Stale QEMU binary (old `systemc_ps.c`) | Rebuild QEMU: `./scripts/build_qemu.sh` |
| Garbled or missing A-profile console | Conflicting QEMU serial args | Use platform defaults (`-nographic` only); do not add `-serial mon:stdio` to `basic_cortexA.env` |
| `arm-none-eabi-gcc: command not found` | Toolchain missing | Re-run `./scripts/setup_host.sh --deps-only` or install manually (Step 1) |
| Stale socket | Previous run crashed | `rm -f /tmp/systemc_cosim.sock` |

### Rebuild after pulling qemu_soc changes

If `qemu_soc/qemu/*.c` changed on `git pull`:

```bash
./scripts/build_qemu.sh
```

The build script re-copies sources into the QEMU tree and rebuilds.

### Check prerequisites manually

```bash
./scripts/setup_host.sh --verify
```

---

## Adding a new platform

1. Copy `platforms/basic_cortexM` (or `basic_cortexA`) to `platforms/my_platform/`
2. Edit `cosim_main.cpp`, `firmware/`, and `Makefile`
3. Add `platforms/my_platform.env` (see existing `.env` files for fields)
4. Run: `./scripts/run_platform.sh my_platform`

Details: [`platforms/README.md`](platforms/README.md).

---

## Quick reference — copy/paste

**New machine (script):**

```bash
cd systemc_model
chmod +x scripts/*.sh
./scripts/setup_host.sh --full
source ~/.zshrc
./scripts/run_platform.sh basic_cortexM
```

**New machine (manual):**

```bash
# 1. host packages (see Step 1)
# 2. build SystemC (Step 2)
# 3. export env vars (Step 3)
# 4. from systemc_model/:
./scripts/build_qemu.sh
./scripts/run_platform.sh basic_cortexM
```

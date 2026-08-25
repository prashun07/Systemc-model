#!/usr/bin/env bash
# Build firmware + SystemC platform for a named configuration, then run QEMU cosim.
#
# Usage:
#   ./scripts/run_platform.sh list
#   ./scripts/run_platform.sh <platform>
#
# Examples:
#   ./scripts/run_platform.sh basic_cortexM
#   ./scripts/run_platform.sh basic_cortexA
set -euo pipefail

# shellcheck source=scripts/config.sh
source "$(cd "$(dirname "$0")" && pwd)/config.sh"

usage() {
  cat <<EOF
Usage: $(basename "$0") <platform>
       $(basename "$0") list

Run cosimulation for a platform defined under platforms/*.env

Available platforms:
EOF
  list_platforms
}

if [[ $# -eq 0 ]]; then
  usage
  exit 1
fi

PLATFORM_NAME="$1"

if [[ "${PLATFORM_NAME}" == "list" || "${PLATFORM_NAME}" == "-h" || "${PLATFORM_NAME}" == "--help" ]]; then
  usage
  exit 0
fi

load_platform "${PLATFORM_NAME}"

if [[ ! -x "${QEMU_BIN}" ]]; then
  echo "error: ${QEMU_BIN} not found"
  echo
  echo "Build custom QEMU first (includes systemc-soc / systemc-ps machines):"
  echo "  ./scripts/build_qemu.sh"
  echo
  echo "Homebrew qemu-system-arm cannot be used — it lacks remote-mmio."
  exit 1
fi

if [[ ! -d "${MODEL_DIR}" ]]; then
  echo "error: model directory not found: ${MODEL_DIR}"
  exit 1
fi

if [[ ! -d "${PLATFORM_DIR}" ]]; then
  echo "error: platform directory not found: ${PLATFORM_DIR}"
  exit 1
fi

echo "==> Platform: ${PLATFORM_NAME}"
echo "    model:    ${MODEL_DIR}"
echo "    cosim:    ${PLATFORM_DIR}"
echo "    firmware: ${FIRMWARE_PATH}"
echo "    machine:  ${QEMU_MACHINE} (${QEMU_CPU})"
echo "==> Data path: QEMU guest MMIO -> remote-mmio -> ${SOCKET}"
echo "               -> transactor -> interconnect/bridges -> peripherals"
echo "    PL base:  ${SYSTEMC_PL_BASE}"

echo "==> Building baremetal firmware"
make -C "${FIRMWARE_DIR}"

echo "==> Building SystemC cosim platform"
make -C "${PLATFORM_DIR}" MODEL_DIR="${MODEL_DIR}"

rm -f "${SOCKET}"

export SYSTEMC_PL_BASE

echo "==> Starting SystemC side (TLM) on ${SOCKET}, PL @ ${SYSTEMC_PL_BASE}"
DYLD_LIBRARY_PATH="${SYSTEMC_LIBDIR}:${DYLD_LIBRARY_PATH:-}" \
  "${PLATFORM_BIN}" "${SOCKET}" &
SC_PID=$!

cleanup() {
  kill "${SC_PID}" 2>/dev/null || true
  wait "${SC_PID}" 2>/dev/null || true
  rm -f "${SOCKET}"
}
trap cleanup EXIT

for _ in $(seq 1 50); do
  [[ -S "${SOCKET}" ]] && break
  sleep 0.1
done
if [[ ! -S "${SOCKET}" ]]; then
  echo "error: SystemC cosim socket did not appear"
  exit 1
fi

echo "==> Starting QEMU ${QEMU_MACHINE}"
export SYSTEMC_COSIM_SOCKET="${SOCKET}"

# shellcheck disable=SC2086
"${QEMU_BIN}" \
  -M "${QEMU_MACHINE}" \
  -cpu "${QEMU_CPU}" \
  ${QEMU_EXTRA_ARGS:-} \
  -kernel "${FIRMWARE_PATH}" \
  -semihosting-config enable=on,target=native \
  ${QEMU_MONITOR_ARGS:--monitor none} \
  -nographic

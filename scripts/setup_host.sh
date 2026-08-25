#!/usr/bin/env bash
# One-time host setup for systemc_model cosimulation.
#
# Detects macOS vs Linux and installs build dependencies, SystemC, and shell env.
# Optionally builds custom QEMU (required for systemc-soc / systemc-ps machines).
#
# Usage (from systemc_model/):
#   ./scripts/setup_host.sh              # deps + SystemC + env block
#   ./scripts/setup_host.sh --full       # above + build QEMU
#   ./scripts/setup_host.sh --deps-only
#   ./scripts/setup_host.sh --systemc-only
#   ./scripts/setup_host.sh --qemu-only  # assumes deps + SystemC already done
#   ./scripts/setup_host.sh --verify     # check tools only
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck source=scripts/setup_lib.sh
source "${ROOT}/scripts/setup_lib.sh"

DO_DEPS=1
DO_SYSTEMC=1
DO_ENV=1
DO_QEMU=0
DO_VERIFY=0

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Machine-aware setup for SystemC + QEMU cosim dependencies.

Options:
  --full          Install deps, build SystemC, write env, build QEMU
  --deps-only     Install OS packages only
  --systemc-only  Build/install SystemC only (no OS packages)
  --qemu-only     Run ./scripts/build_qemu.sh only
  --verify        Check required tools and SystemC install
  -h, --help      Show this help

Environment overrides:
  SYSTEMC_VERSION=${SYSTEMC_VERSION:-2.3.4}
  SYSTEMC_INSTALL=${SYSTEMC_INSTALL:-\$HOME/systemc/install}
  QEMU_PREFIX     (see scripts/build_qemu.sh)

EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
  --full)
    DO_QEMU=1
    shift
    ;;
  --deps-only)
    DO_SYSTEMC=0
    DO_ENV=0
    shift
    ;;
  --systemc-only)
    DO_DEPS=0
    shift
    ;;
  --qemu-only)
    DO_DEPS=0
    DO_SYSTEMC=0
    DO_ENV=0
    DO_QEMU=1
    shift
    ;;
  --verify)
    DO_DEPS=0
    DO_SYSTEMC=0
    DO_ENV=0
    DO_VERIFY=1
    shift
    ;;
  -h|--help)
    usage
    exit 0
    ;;
  *)
    setup_die "unknown option: $1 (try --help)"
    ;;
  esac
done

detect_os
setup_log "Host: ${SETUP_OS} / ${SETUP_ARCH} (jobs=${SETUP_NPROC})"
setup_log "Project: ${ROOT}"

chmod +x "${ROOT}/scripts/"*.sh 2>/dev/null || true

if [[ "${DO_VERIFY}" -eq 1 ]]; then
  verify_prerequisites
  setup_log "All prerequisites OK"
  exit 0
fi

if [[ "${DO_DEPS}" -eq 1 ]]; then
  install_host_deps
fi

if [[ "${DO_SYSTEMC}" -eq 1 ]]; then
  build_systemc
fi

if [[ "${DO_ENV}" -eq 1 ]]; then
  write_env_block
fi

if [[ "${DO_QEMU}" -eq 1 ]]; then
  setup_log "Building custom QEMU (this may take a while)..."
  # shellcheck source=/dev/null
  export SYSTEMC_HOME="${SYSTEMC_INSTALL}"
  export SYSTEMC_LIBDIR="${SYSTEMC_INSTALL}/lib"
  "${ROOT}/scripts/build_qemu.sh"
fi

verify_prerequisites
print_next_steps

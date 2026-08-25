# Shared paths and platform configuration for systemc_model.
# Sourced by other scripts — do not execute directly.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLATFORMS="${ROOT}/platforms"
SCRIPTS="${ROOT}/scripts"

PREFIX="${QEMU_PREFIX:-$HOME/qemu-systemc}"
QEMU_BIN="${QEMU:-${PREFIX}/bin/qemu-system-arm}"
SOCKET="${SYSTEMC_COSIM_SOCKET:-/tmp/systemc_cosim.sock}"
SYSTEMC_LIBDIR="${SYSTEMC_LIBDIR:-${SYSTEMC_HOME:-$HOME/systemc/install}/lib}"

# Load platforms/<name>.env (or platforms/<name>/platform.env).
load_platform() {
  local name="$1"
  local env_file=""

  if [[ -f "${PLATFORMS}/${name}.env" ]]; then
    env_file="${PLATFORMS}/${name}.env"
  elif [[ -f "${PLATFORMS}/${name}/platform.env" ]]; then
    env_file="${PLATFORMS}/${name}/platform.env"
  else
    echo "error: unknown platform '${name}'"
    echo
    echo "Available platforms:"
    list_platforms
    exit 1
  fi

  # shellcheck source=/dev/null
  source "${env_file}"

  : "${PLATFORM_DIR:?PLATFORM_DIR must be set in ${env_file}}"
  : "${MODEL_DIR:?MODEL_DIR must be set in ${env_file}}"
  : "${QEMU_MACHINE:?QEMU_MACHINE must be set in ${env_file}}"
  : "${QEMU_CPU:?QEMU_CPU must be set in ${env_file}}"
  : "${SYSTEMC_PL_BASE:?SYSTEMC_PL_BASE must be set in ${env_file}}"
  : "${FIRMWARE_DIR:?FIRMWARE_DIR must be set in ${env_file}}"
  : "${FIRMWARE_ELF:?FIRMWARE_ELF must be set in ${env_file}}"

  if [[ "${PLATFORM_DIR}" != /* ]]; then
    PLATFORM_DIR="${ROOT}/${PLATFORM_DIR}"
  fi
  if [[ "${MODEL_DIR}" != /* ]]; then
    MODEL_DIR="${ROOT}/${MODEL_DIR}"
  fi
  if [[ "${FIRMWARE_DIR}" != /* ]]; then
    FIRMWARE_DIR="${ROOT}/${FIRMWARE_DIR}"
  fi

  PLATFORM_BIN="${PLATFORM_DIR}/cosim_platform"
  FIRMWARE_PATH="${FIRMWARE_DIR}/${FIRMWARE_ELF}"
}

list_platforms() {
  local f name
  shopt -s nullglob
  for f in "${PLATFORMS}"/*.env; do
    [[ -f "$f" ]] || continue
    name="$(basename "$f" .env)"
    [[ "$name" == _* ]] && continue
    echo "  - ${name}"
  done
  for f in "${PLATFORMS}"/*/platform.env; do
    [[ -f "$f" ]] || continue
    name="$(basename "$(dirname "$f")")"
    [[ "$name" == _* ]] && continue
    echo "  - ${name}"
  done
  shopt -u nullglob
}

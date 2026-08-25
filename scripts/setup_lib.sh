# Shared helpers for scripts/setup_host.sh — source only, do not run directly.

SETUP_MARKER_BEGIN="# >>> systemc_model cosim env >>>"
SETUP_MARKER_END="# <<< systemc_model cosim env <<<"

SYSTEMC_VERSION="${SYSTEMC_VERSION:-2.3.4}"
SYSTEMC_INSTALL="${SYSTEMC_INSTALL:-$HOME/systemc/install}"
SYSTEMC_SRC_ROOT="${SYSTEMC_SRC_ROOT:-$HOME/systemc}"

setup_log() {
  echo "==> $*"
}

setup_warn() {
  echo "warning: $*" >&2
}

setup_die() {
  echo "error: $*" >&2
  exit 1
}

detect_os() {
  case "$(uname -s)" in
  Darwin)  SETUP_OS="macos" ;;
  Linux)   SETUP_OS="linux" ;;
  *)       SETUP_OS="unknown" ;;
  esac

  SETUP_ARCH="$(uname -m)"
  SETUP_NPROC="$(command -v nproc >/dev/null && nproc || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

  if [[ "${SETUP_OS}" == "linux" && -f /etc/os-release ]]; then
    # shellcheck source=/dev/null
    . /etc/os-release
    SETUP_LINUX_ID="${ID:-linux}"
    SETUP_LINUX_LIKE="${ID_LIKE:-}"
  else
    SETUP_LINUX_ID=""
    SETUP_LINUX_LIKE=""
  fi
}

shell_rc_file() {
  if [[ -n "${SHELL:-}" ]] && [[ "${SHELL}" == *zsh* ]]; then
    echo "${HOME}/.zshrc"
  else
    echo "${HOME}/.bashrc"
  fi
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1
}

runtime_lib_var() {
  if [[ "${SETUP_OS}" == "macos" ]]; then
    echo "DYLD_LIBRARY_PATH"
  else
    echo "LD_LIBRARY_PATH"
  fi
}

install_macos_deps() {
  setup_log "Installing macOS packages with Homebrew"
  if ! need_cmd brew; then
    setup_die "Homebrew not found. Install from https://brew.sh then re-run this script."
  fi
  brew update
  brew install cmake git ninja pkg-config glib pixman arm-none-eabi-gcc python3
}

install_linux_deps() {
  if need_cmd apt-get; then
    setup_log "Installing Linux packages with apt"
    sudo apt-get update
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
      build-essential cmake git ninja-build pkg-config \
      libglib2.0-dev libpixman-1-dev gcc-arm-none-eabi python3 \
      curl ca-certificates
    return 0
  fi
  if need_cmd dnf; then
    setup_log "Installing Linux packages with dnf"
    sudo dnf install -y gcc gcc-c++ cmake git ninja-build pkgconfig \
      glib2-devel pixman-devel arm-none-eabi-gcc-cs python3 curl
    return 0
  fi
  setup_die "Unsupported Linux distro. Install manually: cmake git ninja pkg-config glib pixman arm-none-eabi-gcc"
}

install_host_deps() {
  detect_os
  case "${SETUP_OS}" in
  macos) install_macos_deps ;;
  linux) install_linux_deps ;;
  *) setup_die "Unsupported OS: $(uname -s)" ;;
  esac
}

build_systemc() {
  local tarball="systemc-${SYSTEMC_VERSION}.tar.gz"
  local src_dir="${SYSTEMC_SRC_ROOT}/systemc-${SYSTEMC_VERSION}"
  local build_dir="${src_dir}/build"

  if [[ -f "${SYSTEMC_INSTALL}/include/systemc.h" ]]; then
    setup_log "SystemC already installed at ${SYSTEMC_INSTALL} (skip)"
    return 0
  fi

  setup_log "Building SystemC ${SYSTEMC_VERSION} -> ${SYSTEMC_INSTALL}"
  mkdir -p "${SYSTEMC_SRC_ROOT}"
  cd "${SYSTEMC_SRC_ROOT}"

  if [[ ! -d "${src_dir}" ]]; then
    curl -fsSL -o "${tarball}" \
      "https://github.com/accellera-official/systemc/archive/refs/tags/${SYSTEMC_VERSION}.tar.gz"
    tar -xzf "${tarball}"
  fi

  cmake -S "${src_dir}" -B "${build_dir}" \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${SYSTEMC_INSTALL}"
  cmake --build "${build_dir}" -j"${SETUP_NPROC}"
  cmake --install "${build_dir}"
}

write_env_block() {
  local rc shell_lib_var
  rc="$(shell_rc_file)"
  shell_lib_var="$(runtime_lib_var)"

  if [[ -f "${rc}" ]] && grep -q "${SETUP_MARKER_BEGIN}" "${rc}"; then
    setup_log "Environment block already in ${rc} (skip)"
    return 0
  fi

  setup_log "Appending environment block to ${rc}"
  cat >>"${rc}" <<EOF

${SETUP_MARKER_BEGIN}
export SYSTEMC_HOME="${SYSTEMC_INSTALL}"
export SYSTEMC_INCLUDE="\${SYSTEMC_HOME}/include"
export SYSTEMC_LIBDIR="\${SYSTEMC_HOME}/lib"
export QEMU_PREFIX="\${HOME}/qemu-systemc"
export PATH="\${QEMU_PREFIX}/bin:\${PATH}"
export ${shell_lib_var}="\${SYSTEMC_LIBDIR}:\${${shell_lib_var}:-}"
${SETUP_MARKER_END}
EOF
  setup_log "Run: source ${rc}"
}

verify_prerequisites() {
  detect_os
  local missing=0
  local t

  for t in cmake git ninja pkg-config arm-none-eabi-gcc curl; do
    if ! need_cmd "${t}"; then
      setup_warn "missing tool: ${t}"
      missing=1
    fi
  done

  if [[ ! -f "${SYSTEMC_INSTALL}/include/systemc.h" ]]; then
    setup_warn "SystemC not found at ${SYSTEMC_INSTALL}"
    missing=1
  fi

  if [[ "${missing}" -ne 0 ]]; then
    setup_die "Prerequisites incomplete — re-run ./scripts/setup_host.sh"
  fi
}

print_next_steps() {
  local rc shell_lib_var
  rc="$(shell_rc_file)"
  shell_lib_var="$(runtime_lib_var)"

  cat <<EOF

Setup complete for ${SETUP_OS} (${SETUP_ARCH}).

1. Reload your shell:
     source ${rc}

2. Build custom QEMU (first time, ~15-30 min):
     ./scripts/build_qemu.sh

   Or re-run:
     ./scripts/setup_host.sh --full

3. Verify QEMU machines:
     \${QEMU_PREFIX:-\$HOME/qemu-systemc}/bin/qemu-system-arm -machine help | grep systemc

4. Run cosimulation:
     ./scripts/run_platform.sh list
     ./scripts/run_platform.sh basic_cortexM
     ./scripts/run_platform.sh basic_cortexA

Note: Homebrew/system QEMU is NOT used. This project needs the custom build.

EOF
}

#!/usr/bin/env bash

set -e
CPM_SPIKE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${CPM_SPIKE_DIR}/scripts/shared_settings.sh"

KBLD_MARCH="-march=${MARCH_ISA}"
# -------------------------------------------------------------
check_in_path() {
  local cmd="$1"

  if [[ -z "$cmd" ]]; then
    echo "-E: No command specified to check."
    exit 1
  fi

  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "-E: Required command not found in PATH: $cmd"
    exit 1
  fi
}

# -------------------------------------------------------------
# Command-line switches
# -------------------------------------------------------------
SKIP_KERNEL=false
SKIP_BUILDROOT=false
SKIP_OPENSBI=false

for arg in "$@"; do
  case "$arg" in
    --purge)
      echo "Purging build artifacts..."

      PURGE_PATHS=(
        "$KERNEL"
        "$KERNEL_BASE"
        "$KERNEL_BASE.tar.xz"
        "$BUILDROOT"
        "$BUILDROOT_BASE"
        "$BUILDROOT_BASE.tar.gz"
        "opensbi"
        "$RV_LINUX"
      )

      for path in "${PURGE_PATHS[@]}"; do
        if [[ -e "$path" ]]; then
          echo "  Removing $path"
          rm -rf "$path"
        fi
      done

      echo "Purge complete."
      exit 0
      ;;
    --skip-kernel)    SKIP_KERNEL=true ;;
    --skip-buildroot) SKIP_BUILDROOT=true ;;
    --skip-opensbi)   SKIP_OPENSBI=true ;;
    *)
      echo "-E: Unknown argument: $arg"
      exit 1
      ;;
  esac
done

# -------------------------------------------------------------
check_in_path "${BM_BASE}-gcc"
check_in_path "${LNX_BASE}-gcc"

mkdir -p "$RV_LINUX"
# -------------------------------------------------------------
# Build OpenSBI
# -------------------------------------------------------------
if ! $SKIP_OPENSBI; then
  cd "$CPM_SPIKE_DIR"

  echo
  echo "Clone or update OpenSBI"
  echo

  if [[ ! -d "$OPENSBI" ]]; then
    git clone "$OPENSBI_URL"
  else
    cd opensbi && git pull && cd ..
  fi

  make -C ${OPENSBI} CROSS_COMPILE="$LNX_CROSS" PLATFORM=generic -j"$(nproc)"

  if [[ ! -f "${OPENSBI}/build/platform/generic/firmware/fw_jump.bin" ]]; then
    echo "-E: opensbi build failure"
    exit 1
  fi

  if [[ ! -f "${OPENSBI}/build/platform/generic/firmware/fw_jump.elf" ]]; then
    echo "-E: opensbi build failure"
    exit 1
  fi

  cp "${OPENSBI}/build/platform/generic/firmware/fw_jump.bin" "$RV_LINUX"
  cp "${OPENSBI}/build/platform/generic/firmware/fw_jump.elf" "$RV_LINUX"
fi

# -------------------------------------------------------------
# Build the kernel
# -------------------------------------------------------------
if ! $SKIP_KERNEL; then
  cd "$CPM_SPIKE_DIR"

  echo
  echo "Downloading kernel"
  echo
  wget "$KERNEL_URL"

  echo
  echo "Extracting kernel (lengthy)"
  echo
  tar xf "$KERNEL_BASE.tar.xz"
  mv "$KERNEL_BASE" "$KERNEL"

  grep -qxF "KBUILD_CFLAGS += ${KBLD_MARCH}" \
      "${KERNEL}/Makefile" \
      || echo "KBUILD_CFLAGS += ${KBLD_MARCH}" >> "${KERNEL}/Makefile"

  mkdir -p "${RV_LINUX}"
  rm -f "${RV_LINUX}/Image"

  make -C "${KERNEL}" CROSS_COMPILE="$LNX_CROSS" ARCH=riscv defconfig
  make -C "${KERNEL}" CROSS_COMPILE="$LNX_CROSS" ARCH=riscv -j"$(nproc)"

  if [[ ! -f "${KERNEL}/arch/riscv/boot/Image" ]]; then
    echo "-E: kernel build failure"
    exit 1
  fi

  cp "${KERNEL}/arch/riscv/boot/Image" "${RV_LINUX}/Image"
fi

# -------------------------------------------------------------
# Build the file system
# -------------------------------------------------------------
if ! $SKIP_BUILDROOT; then
  echo
  echo "Downloading buildroot"
  echo
  wget "$BUILDROOT_URL"

  echo
  echo "Extracting buildroot (lengthy)"
  echo
  tar xf "${BUILDROOT_BASE}.tar.gz"
  mv "$BUILDROOT_BASE" "$BUILDROOT"
  cp "${BUILDROOT_CONFIG}" "${BUILDROOT}/.config"

  #CROSS_COMPILE not needed, it is in .config
  make -C "$BUILDROOT" -j"$(nproc)"

  if [[ ! -f "${BUILDROOT}/output/images/rootfs.cpio" ]]; then
    echo "-E: rootfs.cpio build failure"
    exit 1
  fi

  cp "${BUILDROOT}/output/images/rootfs.cpio" "${RV_LINUX}"

fi


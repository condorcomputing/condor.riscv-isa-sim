#!/usr/bin/env bash

set -e

CPM_SPIKE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${CPM_SPIKE_DIR}/scripts/shared_settings.sh"

# Default paths
KERNEL_IMAGE="${CPM_SPIKE_DIR}/${RV_LINUX}/Image"
ROOTFS_IMAGE="${CPM_SPIKE_DIR}/${RV_LINUX}/rootfs.cpio"
OPENSBI_ELF="${CPM_SPIKE_DIR}/${RV_LINUX}/fw_jump.elf"

# Optional trace output path
TRACE_OUT=""

# Command-line args
while [[ $# -gt 0 ]]; do
  case "$1" in
    --kernel)
      KERNEL_IMAGE="$2"
      shift 2
      ;;
    --rootfs)
      ROOTFS_IMAGE="$2"
      shift 2
      ;;
    --bootloader)
      OPENSBI_ELF="$2"
      shift 2
      ;;
    --trace)
      TRACE_OUT="$2"
      shift 2
      ;;
    *)
      echo "-E: Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

# Assemble Spike command
SPIKE_CMD=(
  "${CPM_SPIKE_DIR}/build/spike"
  --isa="${SPIKE_ISA}"
  --kernel="${KERNEL_IMAGE}"
  --initrd="${ROOTFS_IMAGE}"
  --bootargs="root=/dev/ram rw earlycon=sbi console=hvc0"
)

# Add trace if specified
if [[ -n "$TRACE_OUT" ]]; then
  SPIKE_CMD+=(--stf_trace "$TRACE_OUT" --stf_macro_tracing)
fi

# Append the bootloader ELF
SPIKE_CMD+=("${OPENSBI_ELF}")

# Run Spike
LD_LIBRARY_PATH="${CPM_SPIKE_DIR}/build" "${SPIKE_CMD[@]}"


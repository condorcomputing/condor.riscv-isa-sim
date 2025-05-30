#!/usr/bin/env bash
# Boot linux and optionally trace
#
# To boot linux
#
#   --kernel     <path to kernel,      typ. Image>
#   --rootfs     <path to filesystem,  typ. rootfs cpio>
#   --bootloader <path to boot loader, typ. fw_jump.elf>
# 
# To specify the path to the trace output file
#
#   --trace_out   <trace file output>
#
# To specify the name of the file to trace on linux boot 
#
#   --auto_trace  <elf name, e.g. dhrystone.riscv> 
#
# When --auto_trace is used the elf name will be passed as a boot
# arg. The target elf must be included in the rootfs in the trace_elfs
# directory.
#
# Linux will boot, the trace enabled elf will be executed and Spike
# will exit on detection of the stop macro.
#
set -e

CPM_SPIKE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${CPM_SPIKE_DIR}/scripts/shared_settings.sh"

CPM_SPIKE="${CPM_SPIKE_DIR}/build/spike"

# Default paths
KERNEL_IMAGE="${CPM_SPIKE_DIR}/${RV_LINUX}/Image"
ROOTFS_IMAGE="${CPM_SPIKE_DIR}/${RV_LINUX}/rootfs.cpio"
OPENSBI_ELF="${CPM_SPIKE_DIR}/${RV_LINUX}/fw_jump.elf"

# Optional trace output path and auto traced elf
TRACE_OUT=""
AUTO_TRACE=""

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
    --trace_out)
      TRACE_OUT="$2"
      shift 2
      ;;
    --auto_trace)
      AUTO_TRACE="$2"
      shift 2
      ;;
    *)
      echo "-E: Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

# Default boot args
BOOT_ARGS="root=/dev/ram rw earlycon=sbi console=hvc0"

## Add trace to boot args if specified
#if [[ -n "$AUTO_TRACE" ]]; then
#  BOOT_ARGS+=" trace_elf=${AUTO_TRACE}"
#fi
#
# Assemble base Spike command to boot linux
SPIKE_CMD=(
  "${CPM_SPIKE}"
  --isa="${SPIKE_ISA}"
  --kernel="${KERNEL_IMAGE}"
  --initrd="${ROOTFS_IMAGE}"
  --bootargs="${BOOT_ARGS}"
)

# future --stf_priv_modes "U"
# Add trace switches if trace_out is specified
#              --stf_trace_memory_records
if [[ -n "$TRACE_OUT" ]]; then
  mkdir -p trace_out
  SPIKE_CMD+=(--stf_trace "$TRACE_OUT" 
              --stf_macro_tracing
              --stf_trace_memory_records
  )
fi

# Append the bootloader ELF
SPIKE_CMD+=("${OPENSBI_ELF}")

# Run Spike
LD_LIBRARY_PATH="${CPM_SPIKE_DIR}/build" "${SPIKE_CMD[@]}"


#!/usr/bin/env bash

set -e

CPM_SPIKE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${CPM_SPIKE_DIR}/scripts/shared_settings.sh"

# Verify required variables
[[ -z "$RV_LINUX" ]] && \
  echo "-E: RV_LINUX is not set" && exit 1

[[ -z "$BUILDROOT" ]] && \
  echo "-E: BUILDROOT is not set" && exit 1

# Check dhrystone directory
[[ ! -d "dhrystone" ]] && \
  echo "-E: dhrystone directory not found" && exit 1

# Check buildroot output root
[[ ! -d "$BUILDROOT/output/target/root" ]] && \
  echo "-E: buildroot output root missing" && exit 1

# Build dhrystone binaries
make -C dhrystone bin-linux || {
  echo "-E: Failed to build Dhrystone"; exit 1;
}

# Verify ELF binaries exist
for opt in 1 2 3; do
  elf="dhrystone/bin/dhrystone_opt${opt}.1000.gcc.linux.riscv"
  [[ ! -f "$elf" ]] && \
    echo "-E: Missing $elf" && exit 1
done

# Create output dir and copy ELFs
mkdir -p "${RV_LINUX}/trace_elfs"

cp dhrystone/bin/dhrystone_opt*.1000.gcc.linux.riscv \
   "${RV_LINUX}/trace_elfs/" || {
  echo "-E: Failed to copy Dhrystone ELFs"; exit 1;
}

# Replace in rootfs
rm -rf "${BUILDROOT}/output/target/root/trace_elfs"

cp -r "${RV_LINUX}/trace_elfs" \
      "${BUILDROOT}/output/target/root/" || {
  echo "-E: Failed to copy trace_elfs"; exit 1;
}

# Rebuild rootfs
make -C "$BUILDROOT" -j"$(nproc)" || {
  echo "-E: Buildroot make failed"; exit 1;
}

# Copy final rootfs image
cp "${BUILDROOT}/output/images/rootfs.cpio" \
   "${RV_LINUX}/trace_rootfs.cpio" || {
  echo "-E: Failed to copy rootfs.cpio"; exit 1;
}


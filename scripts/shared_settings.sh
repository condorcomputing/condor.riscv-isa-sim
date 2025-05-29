#!/usr/bin/env bash

# ------------------------------------------------------------------
# Compilers
# ------------------------------------------------------------------
BM_BASE="riscv64-unknown-elf"
BM_TARBALL="riscv-embecosm-embedded-ubuntu2204-20250309.tar.gz"
BM_TARBALL_URL="https://buildbot.embecosm.com/job/riscv-embedded-ubuntu2204/76/artifact/${BM_TARBALL}"

LNX_BASE="riscv64-unknown-linux-gnu"
LNX_TARBALL="riscv64-embecosm-linux-gcc-ubuntu2204-20240407.tar.gz"
LNX_TARBALL_URL="https://buildbot.embecosm.com/job/riscv64-linux-gcc-ubuntu2204/20/artifact/${LNX_TARBALL}"

LNX_CROSS="riscv64-unknown-linux-gnu-"
# ------------------------------------------------------------------
# Kernel
# ------------------------------------------------------------------
KERNEL_URL="https://www.kernel.org/pub/linux/kernel/v5.x/linux-5.8.4.tar.xz"
KERNEL_BASE="linux-5.8.4"
KERNEL="kernel"

# ------------------------------------------------------------------
# Buildroot
# ------------------------------------------------------------------
BUILDROOT_URL="https://buildroot.org/downloads/buildroot-2025.02.tar.gz"
BUILDROOT_BASE="buildroot-2025.02"
BUILDROOT="buildroot"
BUILDROOT_CONFIG="${CPM_SPIKE_DIR}/scripts/config-buildroot-2025.02"

# ------------------------------------------------------------------
# OpenSBI
# ------------------------------------------------------------------
OPENSBI_URL="https://github.com/riscv/opensbi.git"
OPENSBI="opensbi"

# ------------------------------------------------------------------
# Linux collateral storage
# ------------------------------------------------------------------
RV_LINUX="riscv-linux"

# ------------------------------------------------------------------
# ISA strings
# ------------------------------------------------------------------
# These are slightly different, for spike put back the zicbop 
#SPIKE_ISA="rv64imafdc_h_sscofpmf_sstc_svinval_svnapot_svpbmt_zawrs_zba_zbb_zbc_zbs_zfa_zfh_zfhmin_zicbop_zicbom_zicboz_zicntr_zifencei_zicond_zihintntl_zihintpause_zihpm_zkt_zk_zkn_zknd_zkne_zknh_zbkb_zbkc_zbkx"
SPIKE_ISA=rv64imafdc_zba_zbb_zbc_zbs_zicntr_zicond_zifencei
# For building the kernel with GCC 13 we remove zicbop due to internal error
# See readme.
#MARCH_ISA="rv64imafdc_h_sscofpmf_sstc_svinval_svnapot_svpbmt_zawrs_zba_zbb_zbc_zbs_zfa_zfh_zfhmin_zicbom_zicboz_zicntr_zifencei_zicond_zihintntl_zihintpause_zihpm_zkt_zk_zkn_zknd_zkne_zknh_zbkb_zbkc_zbkx"
MARCH_ISA=rv64imafdc_zba_zbb_zbc_zbs_zicntr_zicond_zifencei

# ------------------------------------------------------
# This is a convenience script, commented out examples of 
# common operations. Functionality of these examples is not
# maintained.
#
# This is setup to be run from ./build
# ------------------------------------------------------

# ------------------------------------------------------
# Verify configure --with
# ------------------------------------------------------
# ../configure --prefix="$(pwd)/../install" 
#  --with-mavis=/home/jeff/Development/jeffnye-gh/riscv-perf-model/mavis \
#  --with-stf_lib=/home/jeff/Development/jeffnye-gh/riscv-perf-model/stf_lib
#  --with-stf_tools=/home/jeff/Development/jeffnye-gh/stf_tools

# ------------------------------------------------------
# Run an stf standalone, without wrappers
# ------------------------------------------------------
# ./spike \
# --isa=rv64gc_H_zba_zbb_zbc_zbs  \
# --stf_trace test.stf \
# --stf_macro_tracing \
# --stf_force_zero_sha \
# ../install/share/stf_tests/force_zero_sha.riscv

# ------------------------------------------------------
# Boot linux w/ tracing 
# ------------------------------------------------------
#bash scripts/boot-linux.sh \
#  --rootfs ./riscv-linux/trace_rootfs.cpio \
#  --trace_out  ./trace_out/linux_trace.zstf

# ------------------------------------------------------
# Boot linux w/ automatic tracing 
# ------------------------------------------------------
#SPIKE_ISA="rv64imafdc_zba_zbb_zbc_zbs_zicntr_zicond_zifencei"
#KERNEL_IMAGE="../riscv-linux/Image"
#ROOTFS_IMAGE="../riscv-linux/trace_rootfs.cpio"
#BOOT_ARGS="root=/dev/ram rw earlycon=sbi console=hvc0 trace_elf=dhrystone_opt3.1000.gcc.linux.riscv"
#OPENSBI_ELF="../riscv-linux/fw_jump.elf"
#
#mkdir -p "../trace_out"
#
#./spike \
#  --stf_trace ../trace_out/linux_auto_trace.zstf \
#  --stf_macro_tracing \
#  --stf_trace_memory_records \
#  --stf_exit_on_stop_opc \
#  --isa="${SPIKE_ISA}" \
#  --kernel="${KERNEL_IMAGE}" \
#  --initrd="${ROOTFS_IMAGE}" \
#  --bootargs="${BOOT_ARGS}" \
#  ${OPENSBI_ELF}

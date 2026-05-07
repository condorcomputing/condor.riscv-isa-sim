MAKEFLAGS += --no-print-directory
#MAKEFLAGS += --silent
# -----------------------------------------------------
# spike requires canonical order rv64imafdc
#
# Note rva23(v), below, are approximations of the RVA23 
# extension list, there is no currently defined succinct 
# compiler switch for rva23
#
# Removed for gcc
#   n/a   nothing removed for gcc 14
# Removed for spike
#   ssstateen smaia smstateen : spike does not like these 
#
# -----------------------------------------------------
# ARCH_CFG 'rva23' 'rva23v' 'simple'
# TOOL_CFG 'gcc'  only gcc is supported in this version
#
#
# Override examples: 
#
# make clean
# make ARCH_CFG=simple 
# make ARCH_CFG=simple bin-linux
#
# Most combinations have not been tested 
# -----------------------------------------------------
TOOL_CFG  = gcc
#ARCH_CFG  ?= rva23
# RVA23 is not the default until mavis copies have been updated
ARCH_CFG  ?= simple
CPM_SPIKE_EXE = ../build/spike
# -----------------------------------------------------
# -----------------------------------------------------
SIMPLE_ARCH = rv64imafdc_zba_zbb_zbc_zbs_zifencei
# -----------------------------------------------------
# FIXME: This can be simplified
ifeq ($(ARCH_CFG), rva23)
RV_MARCH ?= rv64imafdc_h_sscofpmf_sstc_svinval_svnapot_svpbmt_zawrs_zba_zbb_zbc_zbs_zfa_zfh_zfhmin_zicbom_zicbop_zicboz_zicntr_zifencei_zicond_zihintntl_zihintpause_zihpm_zkt_zk_zkn_zknd_zkne_zknh_zbkb_zbkc_zbkx

else ifeq ($(ARCH_CFG),rva23v)
RV_MARCH ?= rv64imafdcv_h_sscofpmf_sstc_svinval_svnapot_svpbmt_zawrs_zba_zbb_zbc_zbs_zfa_zfh_zfhmin_zicbom_zicbop_zicboz_zicntr_zifencei_zicond_zihintntl_zihintpause_zihpm_zkt_zk_zkn_zknd_zkne_zknh_zbkb_zbkc_zbkx

else ifeq ($(ARCH_CFG),simple)
RV_MARCH ?= $(SIMPLE_ARCH)
else
$(error Unsupported ARCH_CFG '$(ARCH_CFG)'. Must be 'rva23' or 'simple')
endif

# -----------------------------------------------------
# GCC TOOLS
# -----------------------------------------------------
ifeq ($(TOOL_CFG),gcc)

RV_MABI    = lp64d
RV_MCPU    =

BM_BASE    = riscv64-unknown-elf
BM_OBJD    = $(BM_BASE)-objdump
BM_OBJC    = $(BM_BASE)-objcopy
BM_CC      = $(BM_BASE)-gcc

LNX_BASE   = riscv64-unknown-linux-gnu
LNX_OBJD   = $(LNX_BASE)-objdump
LNX_OBJC   = $(LNX_BASE)-objcopy
LNX_CC     = $(LNX_BASE)-gcc

FUNC       = -ffast-math \
             -funsafe-math-optimizations \
             -finline-functions \
             -fno-common \
             -fno-builtin-printf
FLTO = -flto

else
$(error Unsupported TOOL_CFG '$(TOOL_CFG)'. Must be 'gcc')
endif
# -----------------------------------------------------
# Final settings
# -----------------------------------------------------
RV_ARCH      = $(RV_MCPU) -march=$(RV_MARCH)      -mabi=$(RV_MABI)
LLVM_RV_ARCH = $(RV_MCPU) -march=$(LLVM_RV_MARCH) -mabi=$(RV_MABI)
# -----------------------------------------------------
BASIC_BM = -nostdlib -nostartfiles
M_MODEL  = -mcmodel=medany 

FAST_CFLAGS  = -O3 $(FUNC) $(FLTO) -fno-tree-loop-distribute-patterns
BM_FLAGS  = $(RV_ARCH) $(BASIC_BM) $(M_MODEL) $(FAST_CFLAGS)
LNX_FLAGS = $(RV_ARCH)             $(M_MODEL) $(FAST_CFLAGS)

BM_FILE_EXT  = $(TOOL_CFG).bare.riscv
LNX_FILE_EXT = $(TOOL_CFG).linux.riscv

# ISA ARCH
# Specialization no longer necessary, can use plain RV_MARCH
ISA_MARCH := $(RV_MARCH)

# -----------------------------------------------------
# objdump common flags
# -----------------------------------------------------
OFLAGS = --disassemble-all --disassemble-zeroes --section=.text \
         --section=.text.startup --section=.text.init  --section=.data \
         -Mnumeric,no-aliases
# -----------------------------------------------------
LNK_SCR = -T./linker/linker.ld
# -----------------------------------------------------
STF_EXT=zstf
# -----------------------------------------------------
# Spike opts
# -----------------------------------------------------
# Log only version of options
SPIKE_LOG_OPTS = --isa=$(ISA_MARCH) -l --log=$(L)
# STF switches added
QT=--quiet
SPIKE_STF_OPTS = $(QT) --isa=$(ISA_MARCH) --stf_trace $(S) --stf_macro_tracing

# -----------------------------------------------------
# DHRYSTONE ITERATIONS - used by multiple targets
# -----------------------------------------------------
DHRYSTONE_ITERATIONS=1000
ITRS=$(DHRYSTONE_ITERATIONS)
DHRYSTONE_BENCHMARKS = dhrystone_opt1.$(ITRS) dhrystone_opt2.$(ITRS) \
                       dhrystone_opt3.$(ITRS)

#BM_BASE = riscv64-unknown-elf
#BM_CC = $(RV_GNU_BAREMETAL_TOOLS)/bin/$(BM_BASE)-gcc
BM_CC = riscv64-unknown-elf-gcc
RV_MARCH_BASE = rv64gc
RV_MARCH_EXT  = zba_zbb_zbc_zbs
RV_MARCH = -march=$(RV_MARCH_BASE)_$(RV_MARCH_EXT)
RV_MABI  = -mabi=lp64d
RV_ARCH  = $(RV_MCPU) $(RV_MARCH) $(RV_MABI)

LNK_SCR = -Ttest_main.ld
INSTALL_DIR = ../../install/share/stf_tests

SRCS = $(wildcard *.S)
RISCV_FILES = $(SRCS:%.S=$(INSTALL_DIR)/%.riscv)

default: $(RISCV_FILES)

$(INSTALL_DIR):
	mkdir -p $(INSTALL_DIR)

$(INSTALL_DIR)/%.riscv: %.S riscv-main.c | $(INSTALL_DIR)
	preprocessed=preprocessed_$*.Sp; \
	asm=asm_$*; \
    $(BM_CC) -E $< -o $$preprocessed; \
	sed '/^[^#]/ {s/^/"/; s/$$/\\n\\t"/}; /^[#]/d' $$preprocessed > $$asm; \
	$(BM_CC) \
		-march=rv64gch_zba_zbb_zbc_zbs  \
		-mabi=lp64d -nostdlib -nostartfiles -mcmodel=medany  -O3 -ffast-math -funsafe-math-optimizations -finline-functions -fno-common -fno-builtin-printf -flto -fno-tree-loop-distribute-patterns \
		-DASM_INCLUDE=$$asm \
		-DPREALLOCATE=1 -D__riscv=1 -Dzba_ENABLED=1 -Dzbb_ENABLED=1 -Dzbc_ENABLED=1 -Dzbs_ENABLED=1 \
		-static -std=gnu99 $(LNK_SCR) \
		crt/crt.S riscv-main.c -o $@ -lm -lgcc; \
	rm $$preprocessed; \
	rm $$asm

clean:
	rm -f $(INSTALL_DIR)/*.riscv
	rm -f preprocessed_*.Sp
	rm -f asm_*
	rm -f *.o
	rm -f *.a

.PHONY: default clean bin-bare-metal

include ./Vars.mk

# top level bin directory
BIN    = ./bin
SRC    = src/dhry_1.c src/dhry_2.c src/strcmp.S
BM_SRC = src/memcpy.c src/printf.c src/traps.c src/crt.S 

BM_OPT1_OBJ =  obj/dhry_1_opt1.$(TOOL_CFG).bare.o \
               obj/dhry_2_opt1.$(TOOL_CFG).bare.o \
               obj/memcpy_opt1.$(TOOL_CFG).bare.o \
               obj/printf_opt1.$(TOOL_CFG).bare.o \
               obj/strcmp_opt1.$(TOOL_CFG).bare.o \
               obj/traps_opt1.$(TOOL_CFG).bare.o  \
               obj/crt_opt1.$(TOOL_CFG).bare.o 

LNX_OPT1_OBJ = obj/dhry_1_lnx_opt1.$(TOOL_CFG).o \
               obj/dhry_2_lnx_opt1.$(TOOL_CFG).o \
               obj/strcmp_lnx_opt1.$(TOOL_CFG).o

HDR      = inc/dhry.h
INC      = -I./inc -I../riscv-tests/env
STD      = -static -std=gnu99
LNX_LIBS = -lm -lgcc

ITRS=$(DHRYSTONE_ITERATIONS)

BM_DEF = -DPREALLOCATE=1 -D__riscv=1 \
         -DCONDOR_DRY_OPT3 \
         -DCONDOR_DRY_NO_MALLOC \
         -DCONDOR_DRY_NO_PRINTF \
         -DCONDOR_DRY_MCYCLE \
         -DDHRY_ITERS=$(ITRS)

LNX_DEF = -DTIME -DNOENUM -D__linux=1 -DDHRY_ITERS=$(ITRS)

G_WARN = -Wno-builtin-declaration-mismatch \
         -Wno-implicit-function-declaration \
         -Wno-pointer-to-int-cast \
         -Wno-implicit-int \
         -Wl,--no-warn-rwx-segment

ifneq ($(TOOL_CFG),gcc)
$(error Unsupported TOOL_CFG '$(TOOL_CFG)'. Must be 'gcc')
endif

DRY_BM_CMN  = $(RV_ARCH) $(BASIC_BM) $(M_MODEL) $(STD) $(FUNC) 
DRY_LNX_CMN = $(RV_ARCH)             $(M_MODEL) $(STD) $(FUNC)

BM_OPT1  = -O2 -fno-inline $(DRY_BM_CMN)            $(BM_DEF) $(INC)
BM_OPT2  = -O3             $(DRY_BM_CMN)            $(BM_DEF) $(INC)
BM_OPT3  = -O3             $(DRY_BM_CMN) $(GCC_LTO) $(BM_DEF) $(INC)

LNX_OPT1  = -O2 -fno-inline $(DRY_LNX_CMN)            $(LNX_DEF) $(INC)
LNX_OPT2  = -O3             $(DRY_LNX_CMN)            $(LNX_DEF) $(INC)
LNX_OPT3  = -O3             $(DRY_LNX_CMN) $(GCC_LTO) $(LNX_DEF) $(INC)

# -------------------------------------------------------------------------
default: bin-bare-metal

bin-bare-metal: $(BIN)/dhrystone_opt1.$(ITRS).$(BM_FILE_EXT)  \
                $(BIN)/dhrystone_opt2.$(ITRS).$(BM_FILE_EXT)  \
                $(BIN)/dhrystone_opt3.$(ITRS).$(BM_FILE_EXT)   

bin-linux: $(BIN)/dhrystone_opt1.$(ITRS).$(LNX_FILE_EXT)  \
           $(BIN)/dhrystone_opt2.$(ITRS).$(LNX_FILE_EXT)  \
           $(BIN)/dhrystone_opt3.$(ITRS).$(LNX_FILE_EXT)   

# ----------------------------------------------------------------------
# Bare metal
# ----------------------------------------------------------------------
# BM - OPT1
obj/crt_opt1.$(TOOL_CFG).bare.o:  src/crt.S
	@mkdir -p obj
	$(BM_CC) -c $(BM_OPT1) $(G_WARN) $< -o $@

obj/dhry_1_opt1.$(TOOL_CFG).bare.o:  src/dhry_1.c $(HDR)
	@mkdir -p obj
	$(BM_CC) -c $(BM_OPT1) $(G_WARN) $< -o $@

obj/dhry_2_opt1.$(TOOL_CFG).bare.o:  src/dhry_2.c $(HDR)
	@mkdir -p obj
	$(BM_CC) -c $(BM_OPT1) $(G_WARN) $< -o $@

obj/memcpy_opt1.$(TOOL_CFG).bare.o:  src/memcpy.c $(HDR)
	@mkdir -p obj
	$(BM_CC) -c $(BM_OPT1) $(G_WARN) $< -o $@

obj/printf_opt1.$(TOOL_CFG).bare.o:  src/printf.c $(HDR)
	@mkdir -p obj
	$(BM_CC) -c $(BM_OPT1) $(G_WARN) $< -o $@

obj/strcmp_opt1.$(TOOL_CFG).bare.o:  src/strcmp.S $(HDR)
	@mkdir -p obj
	$(BM_CC) -c $(BM_OPT1) $(G_WARN) $< -o $@

obj/traps_opt1.$(TOOL_CFG).bare.o:  src/traps.c $(HDR)
	@mkdir -p obj
	$(BM_CC) -c $(BM_OPT1) $(G_WARN) $< -o $@

$(BIN)/dhrystone_opt1.$(ITRS).$(BM_FILE_EXT): $(BM_OPT1_OBJ)
	@echo "compiling $@"
	@mkdir -p bin
	$(BM_CC) $(BM_OPT1) $(LNK_SCR) $(G_WARN) $^ -o $@

# ----------------------------------------------------------------------
# BM - OPT2
$(BIN)/dhrystone_opt2.$(ITRS).$(BM_FILE_EXT): $(SRC) $(HDR)
	@echo "compiling $@"
	@mkdir -p bin
	$(BM_CC) $(BM_OPT2) $(LNK_SCR) $(G_WARN) \
           $(SRC) $(BM_SRC) -o $@

# ----------------------------------------------------------------------
# BM - OPT3
$(BIN)/dhrystone_opt3.$(ITRS).$(BM_FILE_EXT): $(SRC) $(HDR)
	@echo "compiling $@"
	@mkdir -p bin
	$(BM_CC) $(BM_OPT3) $(LNK_SCR) $(G_WARN) \
           $(SRC) $(BM_SRC) -o $@

# ----------------------------------------------------------------------
# LNX - OPT1
obj/dhry_1_lnx_opt1.$(TOOL_CFG).o:  src/dhry_1.c $(HDR)
	@mkdir -p obj
	$(LNX_CC) -c $(LNX_OPT1) $(G_WARN) $< -o $@

obj/dhry_2_lnx_opt1.$(TOOL_CFG).o:  src/dhry_2.c $(HDR)
	@mkdir -p obj
	$(LNX_CC) -c $(LNX_OPT1) $(G_WARN) $< -o $@

obj/strcmp_lnx_opt1.$(TOOL_CFG).o:  src/strcmp.S $(HDR)
	@mkdir -p obj
	$(LNX_CC) -c $(LNX_OPT1) $(G_WARN) $< -o $@

$(BIN)/dhrystone_opt1.$(ITRS).$(LNX_FILE_EXT): $(LNX_OPT1_OBJ)
	@echo "compiling $@"
	@mkdir -p bin
	$(LNX_CC) $(LNX_OPT1) $(G_WARN) $^ -o $@ $(LNX_LIBS)
	
# ----------------------------------------------------------------------
# LNX - OPT2
$(BIN)/dhrystone_opt2.$(ITRS).$(LNX_FILE_EXT): $(SRC) $(HDR)
	@echo "compiling $@"
	@mkdir -p bin
	$(LNX_CC) $(LNX_OPT2) $(G_WARN) $(SRC) -o $@ $(LNX_LIBS)

# ----------------------------------------------------------------------
# LNX - OPT3
$(BIN)/dhrystone_opt3.$(ITRS).$(LNX_FILE_EXT): $(SRC) $(HDR)
	@echo "compiling $@"
	@mkdir -p bin
	$(LNX_CC) $(LNX_OPT3) $(G_WARN) $(SRC) -o $@ $(LNX_LIBS)
	
# ----------------------------------------------------------------------
help-%:
	@echo $* = $($*)

clean:
	rm -f obj/* bin/*


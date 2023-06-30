# TODO RISCV environment variable must be set
ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR := $(ROOT_DIR)build
RISCV := /home/users/niccolo.nicolosi/chipyard/.conda-env/riscv-tools

CC=$(RISCV)/bin/riscv64-unknown-elf-gcc
OBJCOPY=$(RISCV)/bin/riscv64-unknown-elf-objcopy
OBJDUMP=$(RISCV)/bin/riscv64-unknown-elf-objdump
CFLAGS=-static -fno-common -fno-builtin-printf -specs=htif_nano.specs -march=rv64ima -mcmodel=medany -O0 -std=gnu11 -Wall -nostartfiles
CFLAGS+= -fno-common -g -DENTROPY=0 -mabi=lp64 -DNONSMP_HART=0
CFLAGS+= -I $(ROOT_DIR)include -I./utils -I./generated
LFLAGS=-static -L $(ROOT_DIR)linker -T linker.ld -nostdlib

PBUS_CLK ?= 1000000 # default to 1MHz but really should be overridden

default: elf bin dump

elf := $(BUILD_DIR)/compiled.elf
$(elf): head.S sygs_hostcode.c utils/kprintf.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -DTL_CLK="$(PBUS_CLK)UL" $(LFLAGS) -o $@ head.S sygs_hostcode.c utils/kprintf.c -lm

.PHONY: elf
elf: $(elf)

bin := $(BUILD_DIR)/compiled.bin
$(bin): $(elf)
	mkdir -p $(BUILD_DIR)
	$(OBJCOPY) -O binary --change-addresses=-0x80000000 $< $@

.PHONY: bin
bin: $(bin)

dump := $(BUILD_DIR)/compiled.symt
$(dump): $(elf)
	$(OBJDUMP) -t $< > $@

.PHONY: dump
dump: $(dump)

.PHONY: clean
clean::
	rm -rf $(BUILD_DIR)

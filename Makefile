LAB := 6

V := @
PROJECT_DIR := .
BUILD_DIR := $(PROJECT_DIR)/build
KERNEL_IMG := $(BUILD_DIR)/kernel.img
SD_CARD_PATH := $(BUILD_DIR)/sd.img
QEMU := qemu-system-aarch64
_QEMU := $(PROJECT_DIR)/scripts/qemu/qemu_wrapper.sh $(QEMU)
QEMU_GDB_PORT := 1234
QEMU_OPTS := -machine raspi3b -nographic -serial null -serial mon:stdio -m size=1G -kernel $(KERNEL_IMG) -drive if=sd,format=raw,file=$(SD_CARD_PATH)
GDB := gdb-multiarch
CHBUILD := $(PROJECT_DIR)/chbuild

.PHONY: all
all: build

.PHONY: defconfig build clean distclean

defconfig:
	$(V)$(CHBUILD) defconfig

build:
	$(V)test -f $(PROJECT_DIR)/.config || $(CHBUILD) defconfig
	$(V)$(CHBUILD) rebuild && dd if=/dev/zero of=$(BUILD_DIR)/sd.img bs=512 count=32768

clean:
	$(V)$(CHBUILD) clean

distclean:
	$(V)$(CHBUILD) distclean

.PHONY: qemu qemu-gdb gdb

qemu:
	$(V)$(_QEMU) $(QEMU_OPTS)

qemu-gdb:
	$(V)$(_QEMU) -S -gdb tcp::$(QEMU_GDB_PORT) $(QEMU_OPTS)

gdb:
	$(V)gdb-multiarch -x $(PROJECT_DIR)/.gdbinit

.PHONY: grade

grade:
	$(V)test -f $(PROJECT_DIR)/.config && cp $(PROJECT_DIR)/.config $(PROJECT_DIR)/.config.bak
	$(V)$(PROJECT_DIR)/scripts/grade/lab$(LAB).sh
	$(V)test -f $(PROJECT_DIR)/.config.bak && mv $(PROJECT_DIR)/.config.bak $(PROJECT_DIR)/.config

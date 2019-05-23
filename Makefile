# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause

MAKE = make
PWD = $(shell pwd)
KVER  := $(shell uname -r)
KSRC := /lib/modules/$(KVER)/build
CONFIG_PLATFORM_I386_PC = y

obj-$(CONFIG_RTW88_CORE)	+= rtw88.o
rtw88-y += main.o \
	   mac80211.o \
	   util.o \
	   debug.o \
	   tx.o \
	   rx.o \
	   mac.o \
	   phy.o \
	   efuse.o \
	   fw.o \
	   ps.o \
	   sec.o \
	   regd.o

rtw88-$(CONFIG_RTW88_8822BE)	+= rtw8822b.o rtw8822b_table.o
rtw88-$(CONFIG_RTW88_8822CE)	+= rtw8822c.o rtw8822c_table.o

obj-$(CONFIG_RTW88_PCI)		+= rtwpci.o
rtwpci-objs			:= pci.o

ifeq ($(CONFIG_PLATFORM_I386_PC), y)
KSRC = /lib/modules/$(shell uname -r)/build
SUBARCH := $(shell uname -m | sed -e s/i.86/i386/)
ARCH ?= $(SUBARCH)
endif

ifneq ($(KERNELRELEASE),)

else

export CONFIG_RTW88_CORE = m
export CONFIG_RTW88_8822BE = y
export CONFIG_RTW88_8822CE = y
export CONFIG_RTW88_PCI = m

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(PWD)  modules

clean:
	rm -Rf *.o .*.cmd .tmp_versions .*.o.d *.mod.c *.ko modules.order Module.symvers

endif

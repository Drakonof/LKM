MODULE_NAME := led-lkm
MODULE_PERM := 777

SRC_DIR := src
# INCDIR := inc
#BUILD_DIR := .build

SRCS := $(addprefix $(SRC_DIR)/, *.c)

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-y := $(SRCS:.c=.o)

.PHONY: all clean tidy

all: ko # $(BUILDDIR)

#$(BUILDDIR)
#	mkdir -p $@

ko: dt
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

dt: $(addprefix $(SRC_DIR)/, testoverlay.dts)
	dtc -@ -I dts -O dtb -o testoverlay.dtbo $(addprefix $(SRC_DIR)/, testoverlay.dts)

input_arg:
	KCPPFLAGS="-DLED_INPUT_ARG=1" make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	insmod $(MODULE_NAME).ko
	chmod $(MODULE_PERM) /dev/$(MODULE_NAME)

remove:
	rmmod $(MODULE_NAME)

clean:
	-rm -rf .tmp_versions
	-rm -f modules.order .modules.order.cmd Module.symvers .Module.symvers.cmd
	-rm -f $(obj-m) $(obj-m:.o=.mod) $(obj-m:.o=.mod.o) $(obj-m:.o=.mod.c) .$(obj-m:.o=.mod.cmd)
	-rm -f $(addsuffix .cmd,$(addprefix .,$(obj-m)))
	-rm -f $(addsuffix .cmd,$(addprefix .,$(obj-m:.o=.ko)))
	-rm -f $(addsuffix .cmd,$(addprefix .,$(obj-m:.o=.mod.o)))

tidy:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

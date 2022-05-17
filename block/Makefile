BINARY      := blk-dev-skeleton
C_FLAGS     := -Wall

TARGET_PATH := /lib/modules/$(shell uname -r)/kernel/drivers/char

ccflags-y += $(C_FLAGS)

obj-m += $(BINARY).o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	cp $(BINARY).ko $(TARGET_PATH)
	depmod -a

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

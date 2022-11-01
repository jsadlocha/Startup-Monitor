KDIR ?= /lib/modules/`uname -r`/build
obj-m = startupmon.o
startupmon-objs := ./src/startupmon.o ./src/ringbuffer.o ./src/chardriver.o ./src/syscallhook.o
all:
	make -C $(KDIR) M=$(PWD) modules
	#strip startupmon.ko
clean:
	make -C $(KDIR) M=$(PWD) clean
load:
	insmod startupmon.ko
	chmod 777 /dev/startupmon
unload:
	rmmod startupmon.ko
pscnv-y := pscnv_virt_drv.o nouveau_state.o pscnv_ioctl.o pscnv_gem.o \
	nouveau_irq.o pscnv_virt_call.o pscnv_mem.o pscnv_vm.o pscnv_chan.o

obj-m := pscnv.o

SYSSRC = /lib/modules/$(shell uname -r)/build

all:
	+make -C $(SYSSRC) M=$(PWD) modules

clean:
	+make -C $(SYSSRC) M=$(PWD) clean

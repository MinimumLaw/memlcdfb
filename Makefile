#
#
#

ARCH=arm
CROSS_COMPILE=armv5tel-softfloat-linux-gnueabi-
KDIR=/cimc/build/ravion-kernel
ROOT_FS_PATH=/cimc/exportfs/gentoo-armv5tel

obj-m += memlcdfb_spi.o memlcdfb_usb.o

memlcdfb_spi-objs := memlcd_spi.o
memlcdfb_usb-objs := memlcd_usb.o

MAKE_ARGS=ARCH=${ARCH} \
	CROSS_COMPILE=${CROSS_COMPILE} \
	ROOT_FS_PATH=${ROOT_FS_PATH} \
	INSTALL_MOD_PATH=${ROOT_FS_PATH} \
	-C ${KDIR} M=${PWD}

all: modules_install

modules:
	make ${MAKE_ARGS} modules

clean:
	make ${MAKE_ARGS} clean

modules_install: modules
	sudo make ${MAKE_ARGS} modules_install

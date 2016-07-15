#!/bin/sh
echo -e "step0: Setup ENV ..."
export PATH=/opt/gcc-4.3-ls232/bin:$PATH
export ARCH=mips
export CROSS_COMPILE=mipsel-linux-
export MAKE_NPROC=-j`nproc`
export MKIME=mkimage
echo -e "OK :-)\n"

echo -e "step1: Config kernel ..."
if [ ! -f ./.config ]; then
	make ls1c300a_openloongson_robot_defconfig
fi
echo -e "OK :-)\n"

echo -e "step2: Build kernel ..."
make $MAKE_NPROC
if [ $? -eq 0 ]; then
	echo -e "OK :-)\n"
else
	echo -e "Fail !!! :-(\n"
	exit 1
fi

ENTRY_ADDR=`${CROSS_COMPILE}readelf -e vmlinux | grep "Entry point address" | awk '{print $4}'`

echo -e "step3: Make uImage ..."
$MKIME -A mips -O linux -T kernel -C gzip -a 0x80200000 -e $ENTRY_ADDR -n "Linux-3.18" -d arch/mips/boot/compressed/vmlinux.bin.z uImage
if [ $? -eq 0 ]; then
	cp vmlinuz /srv/tftpboot/vmlinuz
	echo -e "OK :-)\n"
else
	echo -e "Fail !!! :-(\n"
	exit 1
fi

exit 0

#!/bin/sh

if ! [ -x /opt/gcc-4.9-ls232 ]  ; then
wget https://mirrors.tuna.tsinghua.edu.cn/loongson/loongson1c_bsp/gcc-4.9/gcc-4.9-ls232.tar.xz -c
if [ $? != 0 ] ; then
wget https://www.anheng.com.cn/loongson/loongson1c_bsp/gcc-4.9/gcc-4.9-ls232.tar.xz -c
fi
tar Jxvf gcc-4.9-ls232.tar.xz -C /opt
fi
PATH=/opt/gcc-4.9-ls232/bin:$PATH
export PATH="/opt/gcc-4.9-ls232/bin:$PATH"

pkgver=`date +%Y%m%d`

apt-get install libncurses5-dev
export CONCURRENCY_LEVEL=8
#make ARCH=mips CROSS_COMPILE=mipsel-linux- menuconfig
export CFLAGS="  -Wno-unused-but-set-variable" 
rm -f debian/stamp/binary/* debian/stamp/install/* debian/stamp/build/*

#make ARCH=mips CROSS_COMPILE=mipsel-linux-  vmlinuz -j 8 

make ARCH=mips CROSS_COMPILE=mipsel-linux-  deb-pkg -j 8 \
LOCALVERSION=-openloongson \
KDEB_PKGVERSION=$pkgver

ls ../linux-*${pkgver}* -l
exit


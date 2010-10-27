#!/bin/sh

DEPMOD=`which depmod`

install -m 600 pcicanII/kvpcicanII.ko /lib/modules/`uname -r`/kernel/drivers/char/
install -m 700 pcicanII/pcicanII.sh /usr/sbin/
grep -v pcicanII /etc/modprobe.conf                  > new_modprobe.conf
echo alias pcicanII kvpcicanII                      >> new_modprobe.conf
echo install kvpcicanII /usr/sbin/pcicanII.sh start >> new_modprobe.conf
echo remove kvpcicanII /usr/sbin/pcicanII.sh stop   >> new_modprobe.conf

cat new_modprobe.conf > /etc/modprobe.conf

$DEPMOD -a
if [ "$?" -ne 0 ] ; then
    echo Failed to execute $DEPMOD -a
fi

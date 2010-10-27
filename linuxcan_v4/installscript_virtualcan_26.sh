#!/bin/sh

DEPMOD=`which depmod`

install -m 600 ./virtualcan/kvvirtualcan.ko /lib/modules/`uname -r`/kernel/drivers/char/
install -m 700 ./virtualcan/virtualcan.sh /usr/sbin/

grep -v virtualcan /etc/modprobe.conf                    > new_modprobe.conf
echo alias virtualcan kvvirtualcan                      >> new_modprobe.conf
echo install kvvirtualcan /usr/sbin/virtualcan.sh start >> new_modprobe.conf
echo remove kvvirtualcan /usr/sbin/virtualcan.sh stop   >> new_modprobe.conf

cat new_modprobe.conf > /etc/modprobe.conf

$DEPMOD -a
if [ "$?" -ne 0 ] ; then
    echo Failed to execute $DEPMOD -a
fi

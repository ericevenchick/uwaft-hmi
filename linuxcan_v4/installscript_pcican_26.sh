#!/bin/sh

DEPMOD=`which depmod`

install -m 600 pcican/kvpcican.ko /lib/modules/`uname -r`/kernel/drivers/char/
install -m 700 pcican/pcican.sh /usr/sbin/

# The space after pcican in the grep below is needed to not match pcicanII.
grep -v "pcican " /etc/modprobe.conf             > new_modprobe.conf
echo alias pcican kvpcican                      >> new_modprobe.conf
echo install kvpcican /usr/sbin/pcican.sh start >> new_modprobe.conf
echo remove kvpcican /usr/sbin/pcican.sh stop   >> new_modprobe.conf

cat new_modprobe.conf > /etc/modprobe.conf

$DEPMOD -a
if [ "$?" -ne 0 ] ; then
    echo Failed to execute $DEPMOD -a
fi

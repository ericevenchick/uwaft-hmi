#!/bin/sh

DEPMOD=`which depmod`

install -m 600 virtualcan/kvvirtualcan.o /lib/modules/`uname -r`/kernel/drivers/char/
install -m 700 virtualcan/virtualcan.sh /usr/sbin/

grep -v virtualcan /etc/modules.conf                     > new_modules.conf
echo alias virtualcan kvvirtualcan                      >> new_modules.conf
echo install kvvirtualcan /usr/sbin/virtualcan.sh start >> new_modules.conf
echo remove kvvirtualcan /usr/sbin/virtualcan.sh stop   >> new_modules.conf

cat new_modules.conf > /etc/modules.conf

$DEPMOD -a
if [ "$?" -ne 0 ] ; then
    echo Failed to execute $DEPMOD -a
fi

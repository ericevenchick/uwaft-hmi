#!/bin/sh

DEPMOD=`which depmod`

install -m 600 pcican/kvpcican.o /lib/modules/`uname -r`/kernel/drivers/char/
install -m 700 pcican/pcican.sh /usr/sbin/

# The space after pcican in the grep below is needed to not match pcicanII.
grep -v "pcican " /etc/modules.conf              > new_modules.conf
echo alias pcican kvpcican                      >> new_modules.conf
echo install kvpcican /usr/sbin/pcican.sh start >> new_modules.conf
echo remove kvpcican /usr/sbin/pcican.sh stop   >> new_modules.conf

cat new_modules.conf > /etc/modules.conf

$DEPMOD -a
if [ "$?" -ne 0 ] ; then
    echo Failed to execute $DEPMOD -a
fi

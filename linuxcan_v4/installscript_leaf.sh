#!/bin/sh

#DEPMOD=`which depmod`
#install -m 700 leaf/leaf.ko /lib/modules/`uname -r`/kernel/drivers/usb/misc/
#install -m 700 leaf/leaf.sh /usr/sbin/
#install -m 777 leaf/leaf /etc/hotplug/usb/
#install -m 644 leaf/leaf.usermap /etc/hotplug/leaf.usermap
#$DEPMOD -a
#if [ "$?" -ne 0 ] ; then
#    echo Failed to execute $DEPMOD -a
#fi


echo "***************************************************"
echo "Kvaser Leaf family currently not supported under kernel < 2.6"
echo "***************************************************"

#!/bin/sh

DEPMOD=`which depmod`
UDEVCTRL=`which udevcontrol`

if [ -z "$UDEVCTRL" ] ; then
  UDEVCTRL="`which udevadm` control"
fi

install -D -m 700 leaf/leaf.ko /lib/modules/`uname -r`/kernel/drivers/usb/misc/leaf.ko
install -m 700 leaf/leaf.sh /usr/sbin/
if [ -d /etc/hotplug ] ; then
  install -m 777 leaf/leaf /etc/hotplug/usb/ ;
  install -m 644 leaf/leaf.usermap /etc/hotplug/leaf.usermap
fi
install -m 644 10-kvaser.rules /etc/udev/rules.d 
$UDEVCTRL reload_rules

$DEPMOD -a
if [ "$?" -ne 0 ] ; then
    echo Failed to execute $DEPMOD -a
fi

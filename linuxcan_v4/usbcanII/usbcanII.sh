#!/bin/bash

# Kvaser CAN driver                     
# usbcanII.sh - start/stop usbcan and create/delete inodes  
# this script can be used if hotplugging doesn't work
# Copyright (C) 2005 Kvaser AB - support@kvaser.com - www.kvaser.com  

LOG=`which logger`


#     
# test kernel version
#     
kernel_ver=`uname -r |awk -F . '{print $2}'` 

case $kernel_ver in
   "6") kv_module_install=modprobe
        ;;
   *)   kv_module_install=insmod
        ;;
esac

#
# install
#
case "$1" in
   start)
      /sbin/rmmod usbcanII
      /sbin/$kv_module_install usbcanII || exit 1
      $LOG -t $0 "Module usbcanII added"
      nrchan=`cat /proc/usbcanII | grep 'total channels' | awk '{print $3}'`
      major=`cat /proc/devices | grep 'usbcanII' | awk '{print $1}'`
      rm -f /dev/usbcanII*
      for (( minor=0 ; minor<$nrchan; minor++ )) ; do
         $LOG -t $0 "Created /dev/usbcanII$minor"
         mknod /dev/usbcanII$minor c $major $minor
      done
      ;;
   stop)
      /sbin/rmmod usbcanII || exit 1
      rm -f /dev/usbcanII*
      $LOG -t $0 "Module usbcanII removed"
      ;;
   *)
      printf "Usage: %s {start|stop}\n" $0
esac

exit 0 

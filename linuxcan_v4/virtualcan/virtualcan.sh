#!/bin/bash

# Kvaser CAN driver                     
# virtualcan.sh - start/stop virtualcan and create/delete device files
# Copyright (C) 2003 Kvaser AB - support@kvaser.com - www.kvaser.com  

#     
# test kernel version
#     
#debug:
#set -x
kernel_ver=`uname -r |awk -F . '{print $2}'` 

devicename=kvvirtualcan
LOG=`which logger`

case $kernel_ver in
   "6") kv_module_install="modprobe -i"
        ;;
   *)   kv_module_install=insmod
        ;;
esac

#
# install
#
case "$1" in
   start)
      /sbin/$kv_module_install $devicename || exit 1
      nrchan=`cat /proc/$devicename | grep 'total channels' | awk '{print $3}'`
      major=`cat /proc/devices | grep ${devicename} | awk '{print $1}'`
      rm -f /dev/${devicename}*
      for (( minor=0; minor < $nrchan ; minor++)) ; do
         $LOG -t $0 "Created /dev/${devicename}${minor}"
         mknod /dev/${devicename}${minor} c ${major} ${minor}
      done
      ;;
   stop)
      /sbin/rmmod $devicename || exit 1
      rm -f /dev/${devicename}*
      $LOG -t $0 "Module $devicename removed"
      ;;
   *)
     printf "Usage: %s {start|stop}\n" $0
esac

#?set +x
exit 0 

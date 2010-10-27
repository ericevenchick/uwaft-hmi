#
# SETTINGS MAKEFILE
#

# all settings should be made in this file

#----------------------------------------
# Choose if we want to build debug: 
# 0 for normal release
# 1 for debug
#----------------------------------------
BUILD_DEBUG=0

#----------------------------------------
# Hardware included in build
#
# To exclude a hardware add a '#' in front 
# of the hardware
#----------------------------------------
#DRIVERS   += lapcan 
DRIVERS   += pcican
DRIVERS   += pcicanII
DRIVERS   += usbcanII
DRIVERS   += leaf
DRIVERS   += virtualcan

#----------------------------------------
# Select kernel source folder
#----------------------------------------
#KERNEL_SOURCE_DIR=/usr/src/linux-`uname -r`
KERNEL_SOURCE_DIR=/lib/modules/`uname -r`/build

#----------------------------------------
# Select driver top directory
#----------------------------------------
#DRIVER_TOP_DIR=/home/test/Sources/linuxcan_v2




# ****** DO NOT CHANGE BELOW THIS LINE ****** 
#*****************************************************************************************************************

# figure out the kernel version
KERNELVERSION := $(shell echo `uname -r` | awk -F'-' '{print $$1}')
KERNELBASE    := $(basename $(KERNELVERSION))
KERNELMINOR   := $(suffix $(KERNELBASE))
KERNELMAJOR   := $(basename $(KERNELBASE))
OLD_MODULES   := $(strip $(foreach V, .0 .1 .2 .3 .4, $(shell [ "$(V)" = "$(KERNELMINOR)" ] && echo yes)))

ifeq ($(OLD_MODULES),yes) # old style make
  KV_KERNEL_VER=2_4
else
  KV_KERNEL_VER=2_6
endif

# export
ifdef DRIVER_TOP_DIR
  export DRIVER_TOP_DIR
endif
export DRIVERS
export BUILD_DEBUG
export KERNEL_SOURCE_DIR
export KV_KERNEL_VER


export KERNELMINOR

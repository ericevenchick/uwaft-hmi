#
# Makefile kernel 2.6
#
ifndef DRIVER_TOP_DIR
  include $(PWD)/config.mak
else
  include $(DRIVER_TOP_DIR)/config.mak
endif

# module name
KV_MODULE_NAME = leaf

# source files
SRCS :=           leafHWIf.c 
SRCS += ../common/VCanOsIf.c 
SRCS += ../common/osif_functions_kernel.c
SRCS += ../common/queue.c


# obj files
OBJS := $(patsubst %.c,%.o,$(SRCS))


#export to second lap
ifeq ($(KV_DEBUG_ON),1)
  export EXTRA_CFLAGS=$(KV_XTRA_CFLAGS_DEBUG_26)
else
  export EXTRA_CFLAGS=$(KV_XTRA_CFLAGS_26)
endif

#------------------------------------------------------

obj-m := $(KV_MODULE_NAME).o
$(KV_MODULE_NAME)-objs := $(OBJS) 

clean:
	rm -f *.o *.ko *.mod.c

cmd_/home/eric/uwaft/linuxcan_v4/pcican/../common/util.o := gcc -Wp,-MD,/home/eric/uwaft/linuxcan_v4/pcican/../common/.util.o.d  -nostdinc -isystem /usr/lib/gcc/i486-linux-gnu/4.4.3/include  -Iinclude  -I/usr/src/linux-headers-2.6.32-25-generic/arch/x86/include -include include/linux/autoconf.h -Iubuntu/include  -D__KERNEL__ -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -m32 -msoft-float -mregparm=3 -freg-struct-return -mpreferred-stack-boundary=2 -march=i586 -mtune=generic -maccumulate-outgoing-args -Wa,-mtune=generic32 -ffreestanding -fstack-protector -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -pipe -Wno-sign-compare -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -Wframe-larger-than=1024 -fno-omit-frame-pointer -fno-optimize-sibling-calls -pg -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fno-dwarf2-cfi-asm -fconserve-stack -DLINUX=1 -D_LINUX=1 -I/home/eric/uwaft/linuxcan_v4/include/ -D_DEBUG=0 -DDEBUG=0 -DLINUX_2_6=1 -DWIN32=0  -DMODULE -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(util)"  -D"KBUILD_MODNAME=KBUILD_STR(kvpcican)"  -c -o /home/eric/uwaft/linuxcan_v4/pcican/../common/.tmp_util.o /home/eric/uwaft/linuxcan_v4/pcican/../common/util.c

deps_/home/eric/uwaft/linuxcan_v4/pcican/../common/util.o := \
  /home/eric/uwaft/linuxcan_v4/pcican/../common/util.c \
  /usr/src/linux-headers-2.6.32-25-generic/arch/x86/include/asm/div64.h \
    $(wildcard include/config/x86/32.h) \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /usr/src/linux-headers-2.6.32-25-generic/arch/x86/include/asm/types.h \
    $(wildcard include/config/x86/64.h) \
    $(wildcard include/config/highmem64g.h) \
  include/asm-generic/types.h \
  include/asm-generic/int-ll64.h \
  /usr/src/linux-headers-2.6.32-25-generic/arch/x86/include/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/linux/posix_types.h \
  include/linux/stddef.h \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
  /usr/src/linux-headers-2.6.32-25-generic/arch/x86/include/asm/posix_types.h \
  /usr/src/linux-headers-2.6.32-25-generic/arch/x86/include/asm/posix_types_32.h \
  /home/eric/uwaft/linuxcan_v4/include/util.h \

/home/eric/uwaft/linuxcan_v4/pcican/../common/util.o: $(deps_/home/eric/uwaft/linuxcan_v4/pcican/../common/util.o)

$(deps_/home/eric/uwaft/linuxcan_v4/pcican/../common/util.o):

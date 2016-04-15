#Makefile for Greenvity Virtual Ethernet Dual Driver (GHDD)
#---------------------------------------------------------------------SOURCES AND INCLUDE PATH START

#Path to directory enclosing the source files for this project. Can be overridden by the Makefile which calls this make file.
#IMPORTANT: If you are calling this makefile from outside its directory, please pass the path to it via this variable.
ifeq ($(enclosing_dir),)
  enclosing_dir= $(shell pwd)
endif

ifeq ($(obj),)
  obj=	.
endif

#If TARGET is not specified at command line, assume it to be x86
ifeq ($(TARGET),)
  TARGET = x86
endif

#If USE_STACK is not specified at command line, assume it to be Linux's Stack
# Valid options for this parameters are: "Greenvity" and "Linux"
ifeq ($(USE_STACK),)
  USE_STACK = Linux
endif

ifeq ($(KERNEL_DIR_ARM),)

ifeq "$(TARGET)" "imx233"
	KERNEL_DIR_ARM = ~/ltib/rpm/BUILD/linux-2.6.31
endif

ifeq "$(TARGET)" "imx287"
	KERNEL_DIR_ARM = /home/greenvity/workdir/i.mx287/ltib/rpm/BUILD/linux-2.6.35.3
endif

ifeq "$(TARGET)" "imx25"
	KERNEL_DIR_ARM = ~/ltib/rpm/BUILD/linux-2.6.31_release
endif
  
endif

ifeq ($(KERNEL_DIR_X86),)
  KERNEL_DIR_X86 = /lib/modules/$(shell uname -r)/build
endif

# Autodetect active ethernet device name
ifeq ($(ETH_ATTACH_TO_X86),)
  ETH_ATTACH_TO_X86 = $(shell ls /sys/class/net | awk /eth[0-9]*/)
endif

GHDD_SRC_DIR1 = host/test
GHDD_SRC_DIR2 = sap
GHDD_SRC_DIR3 = host/crypto
GHDD_SRC_DIR4 = host/lrwpan

GHDD_INCS += -I$(obj) -I$(obj)/$(GHDD_SRC_DIR1) -I$(obj)/$(GHDD_SRC_DIR2) -I$(obj)/$(GHDD_SRC_DIR3) -I$(obj)/$(GHDD_SRC_DIR4)
#---------------------------------------------------------------------SOURCES AND INCLUDE PATH END
#---------------------------------------------------------------------TEXT FORMATTING START

ifeq ($(COLORS_DEFINED),)
  COLORS_DEFINED = yes
  TXTRED    = @tput setaf 1
  TXTGREEN  = @tput setaf 2
  TXTYELLOW = @tput setaf 3
  TXTBLUE   = @tput setaf 4
  TXTPURPLE = @tput setaf 5
  TXTCYAN   = @tput setaf 6
  TXTWHITE  = @tput setaf 7
  TXTRESET  = @tput sgr0
endif

#---------------------------------------------------------------------TEXT FORMATTING END
#---------------------------------------------------------------------TARGETS TYPES START

#In case of ARM, use either of the following compilers
#First one is the version that came with LTIB
#Second one is the one from CodeSourcery that we install seperately (arm-2011.09-70-arm-none-linux-gnueabi.bin)
ifeq "$(TARGET)" "imx233"
  CROSS_COMPILE_ARM = /opt/freescale/usr/local/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-
endif

ifeq "$(TARGET)" "imx25"
  ifeq ($(MAKEARGS),)
    MAKEARGS = ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE_ARM) -C $(KERNEL_DIR_ARM)
  endif
  EXTRA_CFLAGS += $(GHDD_INCS) -D_PLATFORM_LINUX_IMX233_ -D_GHDD_MAC_SAP_ -DMODULE -DCOMMISSIONING
  ifeq "$(USE_STACK)" "Greenvity"
    EXTRA_CFLAGS +=
  endif
  EXTRA_CFLAGS += -DUNAME_KVERSION=2 -DUNAME_KMAJOR_REV=6 -DUNAME_KMINOR_REV=31 -DMODULE
  obj-m		+= ghdd.o
endif

ifeq "$(TARGET)" "imx233"
  ifeq ($(MAKEARGS),)
    MAKEARGS = ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE_ARM) -C $(KERNEL_DIR_ARM)
  endif
  EXTRA_CFLAGS += $(GHDD_INCS) -D_PLATFORM_LINUX_IMX233_ -D_GHDD_MAC_SAP_ -DMODULE
  ifeq "$(USE_STACK)" "Greenvity"
    EXTRA_CFLAGS +=
  endif
  EXTRA_CFLAGS += -DUNAME_KVERSION=2 -DUNAME_KMAJOR_REV=6 -DUNAME_KMINOR_REV=31 -DMODULE
  obj-m		+= ghdd.o
endif

ifeq "$(TARGET)" "imx287"
  CROSS_COMPILE_ARM = /opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-fsl-linux-gnueabi-
  ifeq ($(MAKEARGS),)
    MAKEARGS = ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE_ARM) -C $(KERNEL_DIR_ARM)
  endif
  EXTRA_CFLAGS += $(GHDD_INCS) -D_PLATFORM_LINUX_IMX233_ -D_GHDD_MAC_SAP_ -DMODULE
  ifeq "$(USE_STACK)" "Greenvity"
    EXTRA_CFLAGS +=
  endif
  EXTRA_CFLAGS += -DUNAME_KVERSION=2 -DUNAME_KMAJOR_REV=6 -DUNAME_KMINOR_REV=35 -DCOMMISSIONING -DMODULE
  obj-m		+= ghdd.o
endif


ifeq "$(TARGET)" "x86"
  ifeq ($(MAKEARGS),)
    MAKEARGS = -C $(KERNEL_DIR_X86)
  endif
  EXTRA_CFLAGS +=  $(GHDD_INCS) -D_PLATFORM_LINUX_X86_ -D_GHDD_MAC_SAP_ -DMODULE
  ifeq "$(USE_STACK)" "Greenvity"
    EXTRA_CFLAGS +=
  endif
  EXTRA_CFLAGS += -DUNAME_KVERSION=$(shell uname -r | cut -d. -f1) -DUNAME_KMAJOR_REV=$(shell uname -r | cut -d. -f2) -DUNAME_KMINOR_REV=$(shell uname -r | cut -d. -f3 | cut -d- -f1) -DMODULE
  obj-m		+= ghdd.o
endif
#---------------------------------------------------------------------TARGETS TYPES END
#---------------------------------------------------------------------OBJECTS START
ghdd-objs := 	ghdd_driver.o \
				genl_ghdd_cli.o \
				genl_ghdd_event.o \
				ghdd_tlv.o \
				$(GHDD_SRC_DIR1)/nmm.o \
				$(GHDD_SRC_DIR1)/nmmh1.o \
				$(GHDD_SRC_DIR1)/event.o \
				$(GHDD_SRC_DIR1)/host.o \
				$(GHDD_SRC_DIR1)/timer_irq.o \
				$(GHDD_SRC_DIR3)/pbkdf1.o \
				$(GHDD_SRC_DIR3)/sha2.o \
				$(GHDD_SRC_DIR2)/mac_intf_common.o \
				$(GHDD_SRC_DIR2)/mac_intf.o \
				$(GHDD_SRC_DIR2)/mac_host_intf.o \
				$(GHDD_SRC_DIR4)/lrwpan.o 
#---------------------------------------------------------------------OBJECTS END
#---------------------------------------------------------------------RECIPIES START

#Default compilation recipe
all: build clean-temp-silent

#Clean executable and temporary files
clean :
	clear
	@echo "----------------------------------------------------------"
	$(TXTRED)
	@echo "Cleaning executable and temporary files..."
	$(TXTRESET)
	-rm -f $(enclosing_dir)/*.ko
	-rm -rf $(enclosing_dir)/.tmp_versions
	#-rm -f $(enclosing_dir)/Module.symvers
	#-rm -rf $(enclosing_dir)/modules.order
	-rm -f $(enclosing_dir)/*~ $(enclosing_dir)/*.o $(enclosing_dir)/*.mod.* $(enclosing_dir)/.*.o.* $(enclosing_dir)/.*.ko.cmd
	-rm -f $(enclosing_dir)/$(GHDD_SRC_DIR1)/*~ $(enclosing_dir)/$(GHDD_SRC_DIR1)/*.o $(enclosing_dir)/$(GHDD_SRC_DIR1)/*.mod.* $(enclosing_dir)/$(GHDD_SRC_DIR1)/.*.o.* $(enclosing_dir)/$(GHDD_SRC_DIR1)/.*.ko.cmd
	-rm -f $(enclosing_dir)/$(GHDD_SRC_DIR2)/*~ $(enclosing_dir)/$(GHDD_SRC_DIR2)/*.o $(enclosing_dir)/$(GHDD_SRC_DIR2)/*.mod.* $(enclosing_dir)/$(GHDD_SRC_DIR2)/.*.o.* $(enclosing_dir)/$(GHDD_SRC_DIR2)/.*.ko.cmd
	-rm -f $(enclosing_dir)/$(GHDD_SRC_DIR3)/*~ $(enclosing_dir)/$(GHDD_SRC_DIR3)/*.o $(enclosing_dir)/$(GHDD_SRC_DIR3)/*.mod.* $(enclosing_dir)/$(GHDD_SRC_DIR3)/.*.o.* $(enclosing_dir)/$(GHDD_SRC_DIR3)/.*.ko.cmd
	-rm -f $(enclosing_dir)/$(GHDD_SRC_DIR4)/*~ $(enclosing_dir)/$(GHDD_SRC_DIR4)/*.o $(enclosing_dir)/$(GHDD_SRC_DIR4)/*.mod.* $(enclosing_dir)/$(GHDD_SRC_DIR4)/.*.o.* $(enclosing_dir)/$(GHDD_SRC_DIR4)/.*.ko.cmd
	@echo "----------------------------------------------------------"
	
#Compile the kernel modules
build :
	clear
	@echo "----------------------------------------------------------"
	$(TXTGREEN)
	@echo "Making Greenvity Hybrii Device Driver... (for $(shell uname -r | cut -d. -f1).$(shell uname -r | cut -d. -f2).$(shell uname -r | cut -d. -f3 | cut -d- -f1))"
	@echo "TARGET option selected as $(TARGET) out of (x86 | imx233 | imx287)"
	@echo "USE_STACK option selected as $(USE_STACK) out of (Greenvity | Linux)"
	$(TXTRESET)
	$(MAKE) $(MAKEARGS) SUBDIRS=$(enclosing_dir) modules
	@echo "----------------------------------------------------------"
	
#Clean temporary files without echoing messages to the screen
clean-temp-silent :
	$(TXTYELLOW)
	@echo "Cleaning Temporary Files..."
	$(TXTRESET)
	@-rm -rf $(enclosing_dir)/.tmp_versions
	#@-rm -f $(enclosing_dir)/Module.symvers
	#@-rm -rf $(enclosing_dir)/modules.order
	@-rm -f $(enclosing_dir)/*~ $(enclosing_dir)/*.o $(enclosing_dir)/*.mod.* $(enclosing_dir)/.*.o.* $(enclosing_dir)/.*.ko.cmd
	@-rm -f $(enclosing_dir)/$(GHDD_SRC_DIR1)/*~ $(enclosing_dir)/$(GHDD_SRC_DIR1)/*.o $(enclosing_dir)/$(GHDD_SRC_DIR1)/*.mod.* $(enclosing_dir)/$(GHDD_SRC_DIR1)/.*.o.* $(enclosing_dir)/$(GHDD_SRC_DIR1)/.*.ko.cmd
	@-rm -f $(enclosing_dir)/$(GHDD_SRC_DIR2)/*~ $(enclosing_dir)/$(GHDD_SRC_DIR2)/*.o $(enclosing_dir)/$(GHDD_SRC_DIR2)/*.mod.* $(enclosing_dir)/$(GHDD_SRC_DIR2)/.*.o.* $(enclosing_dir)/$(GHDD_SRC_DIR2)/.*.ko.cmd
	@-rm -f $(enclosing_dir)/$(GHDD_SRC_DIR3)/*~ $(enclosing_dir)/$(GHDD_SRC_DIR3)/*.o $(enclosing_dir)/$(GHDD_SRC_DIR3)/*.mod.* $(enclosing_dir)/$(GHDD_SRC_DIR3)/.*.o.* $(enclosing_dir)/$(GHDD_SRC_DIR3)/.*.ko.cmd
	@-rm -f $(enclosing_dir)/$(GHDD_SRC_DIR4)/*~ $(enclosing_dir)/$(GHDD_SRC_DIR4)/*.o $(enclosing_dir)/$(GHDD_SRC_DIR4)/*.mod.* $(enclosing_dir)/$(GHDD_SRC_DIR4)/.*.o.* $(enclosing_dir)/$(GHDD_SRC_DIR4)/.*.ko.cmd
	@echo "----------------------------------------------------------"
	


#Install Kernel Modules
install : uninstall clear-msgs 
	@echo "----------------------------------------------------------"
	$(TXTBLUE)
	@echo "Installing GHDD Kernel module..."
	$(TXTCYAN)
	@echo "Autodetected active ethernet device name: "$(ETH_ATTACH_TO_X86)
	$(TXTRESET)
	-insmod ghdd.ko attach_to=$(ETH_ATTACH_TO_X86) gv710x_interface="ether"
	@echo "----------------------------------------------------------"

#Uninstall Kernel Modules
uninstall :
	@echo "----------------------------------------------------------"
	$(TXTPURPLE)
	@echo "Uninstalling GHDD Kernel module..."
	$(TXTRESET)
	-rmmod ghdd
	@echo "----------------------------------------------------------"

#Clear screen kernel message buffer
clear-msgs :
	clear
	dmesg -c > /dev/null
	clear
	
#Print colorized kernel message logs and empty the ring buffer
#Requires ccze preinstalled .. use "sudo apt-get install ccze"
kernel-msgs :
	@sudo dmesg -c | ccze --raw-ansi --plugin syslog

#---------------------------------------------------------------------RECIPIES END
#---------------------------------------------------------------------LINKER START
ghdd.o : $(ghdd-objs)
	$(LD) -o ghdd.ko -r $(ghdd-objs)
#---------------------------------------------------------------------LINKER END

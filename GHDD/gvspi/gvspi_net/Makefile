#Makefile for Greenvity Virtual Ethernet Dual Driver (GVEDD)
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

GVEDD_INCS += -I$(obj) -I../ 
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
  ifeq ($(MAKEARGS),)
    MAKEARGS = ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE_ARM) -C $(KERNEL_DIR_ARM)
  endif
  EXTRA_CFLAGS += $(GVEDD_INCS) 
  obj-m		+= gv701x_spi_net.o
endif

ifeq "$(TARGET)" "imx287"
  CROSS_COMPILE_ARM = /opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-fsl-linux-gnueabi-
  ifeq ($(MAKEARGS),)
    MAKEARGS = ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE_ARM) -C $(KERNEL_DIR_ARM)
  endif
  EXTRA_CFLAGS += $(GVEDD_INCS) 
  obj-m		+= gv701x_spi_net.o
endif

ifeq "$(TARGET)" "x86"
  ifeq ($(MAKEARGS),)
    MAKEARGS = -C $(KERNEL_DIR_X86)
  endif
  EXTRA_CFLAGS +=  $(GVEDD_INCS) 
  obj-m		+= gv701x_spi_net.o
endif
#---------------------------------------------------------------------TARGETS TYPES END
#---------------------------------------------------------------------OBJECTS START
#gv701x_spi_net-objs := 	gv701x_spi_net.o 
#---------------------------------------------------------------------OBJECTS END
#---------------------------------------------------------------------RECIPIES START

#Default compilation recipe
all: clean build clean-temp-silent

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
	@echo "----------------------------------------------------------"
	
#Compile the kernel modules
build :
	clear
	@echo "----------------------------------------------------------"
	$(TXTGREEN)
	@echo "Making gv701x_spi_net Driver..."
	@echo "TARGET option selected as $(TARGET) out of (x86 | imx233 | imx287)"
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
	@echo "----------------------------------------------------------"
	


#Install Kernel Modules
install : uninstall clear-msgs 
	@echo "----------------------------------------------------------"
	$(TXTBLUE)
	@echo "Installing gv701x_spi_net Kernel module..."
	$(TXTRESET)
	-insmod gv701x_spi_net.ko 
	@echo "----------------------------------------------------------"

#Uninstall Kernel Modules
uninstall :
	@echo "----------------------------------------------------------"
	$(TXTPURPLE)
	@echo "Uninstalling gv701x_spi_net Kernel module..."
	$(TXTRESET)
	-rmmod gv701x_spi_net
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
gv701x_spi_net.o : $(gv701x_spi_net-objs)
	$(LD) -o gv701x_spi_net.ko -r $(gv701x_spi_net-objs)
#---------------------------------------------------------------------LINKER END

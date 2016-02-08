#Makefile for compiling Userspace Greenvity Production Tool Host Daemon

#Path to directory enclosing the source files for this project. Can be overridden by the Makefile which calls this make file.
#IMPORTANT: If you are calling this makefile from outside its directory, please pass the path to it via this variable.

ifeq ($(enclosing_dir),)
  enclosing_dir= $(dir $(lastword $(MAKEFILE_LIST)))
endif

#If TARGET is not specified at command line, assume it to be x86
ifeq ($(TARGET),)
  TARGET = x86
endif

#Math library required
LIBS = -lm -lpthread -lrt

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

ifeq "$(TARGET)" "imx287"
  #Compiler to be used arm-2011.09-70-arm-none-linux-gnueabi.bin
  #CC=~/CodeSourcery/Sourcery_CodeBench_Lite_for_ARM_GNU_Linux/bin/arm-none-linux-gnueabi-gcc
  #CC=/opt/freescale/usr/local/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-gcc
  #CC=/usr/bin/arm-linux-gnueabi-gcc
  CC=/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-none-linux-gnueabi-gcc
#  CFLAGS= -static -g
   CFLAGS= -Wall -g -fPIC \
	-I$(enclosing_dir)\
#	-D_DUMMY_TEST_
  EXE_NAME=gv_server.out
endif

ifeq "$(TARGET)" "x86"
  #use x86 gcc
  CC=gcc
  CFLAGS= -Wall -g -fPIC \
	-I$(enclosing_dir) \
	-DLINUX_OS
  EXE_NAME=gv_server.out
endif

#---------------------------------------------------------------------TARGETS TYPES END
#---------------------------------------------------------------------OBJECTS START
CFILES = $(enclosing_dir)/findif.c \
	$(enclosing_dir)/dictionary.c \
	$(enclosing_dir)/iniparser.c \
	$(enclosing_dir)/msglog.c \
	$(enclosing_dir)/utils.c \
	$(enclosing_dir)/eth_socket_interface.c \
	$(enclosing_dir)/gvspi_intf.c \
	$(enclosing_dir)/process_state_machine.c \
	$(enclosing_dir)/main.c 

#---------------------------------------------------------------------OBJECTS END
#---------------------------------------------------------------------RECIPIES START

%.o: %.c 
	$(CC) $(CFLAGS) -c $<

all: $(CFILES)
	if [ -f $(shell pwd)/*.out ] ; then  sudo rm $(shell pwd)/*.out ; fi
	@echo "----------------------------------------------------------"
	$(TXTYELLOW)
	@echo "Building Greenvity Production Tool Host Daemon..."
	@echo "TARGET option selected as $(TARGET) out of (x86 | imx287)"
	@echo $(enclosing_dir)
	$(TXTRESET)
	$(CC) $(CFILES) $(CFLAGS) -o $(EXE_NAME) $(LIBS)
	@echo "----------------------------------------------------------"
	#sudo rm /opt/greenvity/server/*.out
	#sudo cp $(shell pwd)/*.out /opt/greenvity/server
	#sudo cp $(shell pwd)/msglog_runtime_config.txt  /opt/greenvity/server	
clean:
	rm $(shell pwd)/*.out

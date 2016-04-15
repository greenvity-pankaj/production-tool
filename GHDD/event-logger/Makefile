#Makefile for compiling userspace Command Line Interface application for GHDD
#---------------------------------------------------------------------SOURCES AND INCLUDE PATH START

#Path to directory enclosing the source files for this project. Can be overridden by the Makefile which calls this make file.
#IMPORTANT: If you are calling this makefile from outside its directory, please pass the path to it via this variable.
ifeq ($(enclosing_dir),)
  enclosing_dir= $(shell pwd)
endif

#If TARGET is not specified at command line, assume it to be x86
ifeq ($(TARGET),)
  TARGET = x86
endif

ifeq ($(obj),)
  obj=	.
endif

#Math library required
LIBS = -lm

GHDD_EVT_SRC_DIR1 = ../ghdd
GHDD_EVT_SRC_DIR2 = ../ghdd/host/test
GHDD_EVT_SRC_DIR3 = ../ghdd/sap
GHDD_EVT_INCS += -I$(obj) -I$(obj)/$(GHDD_EVT_SRC_DIR1) 

#-I$(obj)/$(GHDD_CLI_SRC_DIR2) -I$(obj)/$(GHDD_CLI_SRC_DIR3)


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

ifeq "$(TARGET)" "imx25"
  #Compiler to be used arm-2011.09-70-arm-none-linux-gnueabi.bin
  #CC=~/CodeSourcery/Sourcery_CodeBench_Lite_for_ARM_GNU_Linux/bin/arm-none-linux-gnueabi-gcc
  CC=/opt/freescale/usr/local/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-gcc
  CFLAGS= -Wall -static -g
  EXTRA_CFLAGS += $(GHDD_EVT_INCS) 
  EXE_NAME=ghdd_event_imx25.out
  define CREATE_SYMBOLIC_LINK
  endef
endif

ifeq "$(TARGET)" "imx233"
  #Compiler to be used arm-2011.09-70-arm-none-linux-gnueabi.bin
  #CC=~/CodeSourcery/Sourcery_CodeBench_Lite_for_ARM_GNU_Linux/bin/arm-none-linux-gnueabi-gcc
  CC=/opt/freescale/usr/local/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-gcc
  CFLAGS= -Wall -static -g
  EXTRA_CFLAGS += $(GHDD_EVT_INCS) 
  EXE_NAME=ghdd_event_imx233.out
  define CREATE_SYMBOLIC_LINK
  endef
endif

ifeq "$(TARGET)" "imx287"
  #Compiler to be used arm-2011.09-70-arm-none-linux-gnueabi.bin
  #CC=~/CodeSourcery/Sourcery_CodeBench_Lite_for_ARM_GNU_Linux/bin/arm-none-linux-gnueabi-gcc
  CC=/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-none-linux-gnueabi-gcc
  CFLAGS= -Wall -static -g
  EXTRA_CFLAGS += $(GHDD_EVT_INCS)
  EXE_NAME=ghdd_event_imx287.out
  define CREATE_SYMBOLIC_LINK
  endef
endif

ifeq "$(TARGET)" "x86"
  #use x86 gcc
  CC=gcc
  CFLAGS= -Wall -g
  EXTRA_CFLAGS += $(GHDD_EVT_INCS) 
  EXE_NAME=ghdd_event_x86.out
  define CREATE_SYMBOLIC_LINK
  endef
endif



#---------------------------------------------------------------------TARGETS TYPES END
#---------------------------------------------------------------------OBJECTS START
OBJ = 	$(enclosing_dir)/main.o 

CFILES = $(enclosing_dir)/main.c

#---------------------------------------------------------------------OBJECTS END
#---------------------------------------------------------------------RECIPIES START

%.o: %.c 
	$(CC) $(CFLAGS) -c $<

all: $(CFILES)
	clear
	@echo "----------------------------------------------------------"
	$(TXTRED)
	@echo "Building GHDD Event Logger userspace application..."
	@echo "TARGET option selected as $(TARGET) out of (x86 | imx233 | imx25)"
	$(TXTRESET)
	$(CC) $(CFILES) $(CFLAGS) -o $(EXE_NAME) $(LIBS)
	$(CREATE_SYMBOLIC_LINK)
	@echo "----------------------------------------------------------"

clean:
	rm $(shell pwd)/*.out

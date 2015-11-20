
MUX_CFLAGS += -I./ -I./../ -I./../../../common/ -I./../hal/ 

ifeq ($(SIMULATE),y)
        MUX_CFLAGS += -I./../simu/
endif

ifeq ($(CONFIG_HPGP_HAL),y)
        MUX_CFLAGS += -I./../../../hal/
endif

MUX_OBJS = muxl.o 

all: $(MUX_OBJS)

$(MUX_OBJS):%.o: %.c
	$(CC) -c $(CFLAGS) $(MUX_CFLAGS) $< -o $@ 

clean:
	rm -f *.o	


HAL_CFLAGS += -I./ -I./.. -I./../../../common 

ifeq ($(CONFIG_HPGP_HAL), y)
HAL_CFLAGS += -I./../../../hal
else
HAL_CFLAGS += -I./../simu
endif


HAL_OBJS = hal.o 

all: $(HAL_OBJS)

$(HAL_OBJS):%.o: %.c
	$(CC) -c $(CFLAGS) $(HAL_CFLAGS) $< -o $@ 

clean:
	rm -f *.o	

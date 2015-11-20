
HHAL_CFLAGS += -I./ -I./.. -I./../common -I./../hpgp/src

HHAL_CFLAGS += -I./../hpgp/src/hal


HHAL_OBJS = hal_common.o hal_hpgp.o hal_hpgpRx.o hal_hpgpTx.o 

all: $(HHAL_OBJS)

$(HHAL_OBJS):%.o: %.c
	$(CC) -c $(CFLAGS) $(HHAL_CFLAGS) $< -o $@ 

clean:
	rm -f *.o	

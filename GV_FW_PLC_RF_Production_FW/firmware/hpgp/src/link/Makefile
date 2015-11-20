
LINKL_CFLAGS += -I./ -I./../ -I./../../../common -I./../mux -I./../hal \
		-I./../nma -I./../crypto/

ifeq ($(SIMULATE),y)
	LINKL_CFLAGS += -I./../simu/ 
endif

ifeq ($(CONFIG_HPGP_HAL),y)
	LINKL_CFLAGS += -I./../../../hal 
endif


LINKL_OBJS = linkl.o crm.o nsm.o nam.o akm.o hom.o 

all: $(LINKL_OBJS)

$(LINKL_OBJS):%.o: %.c
	$(CC) -c $(CFLAGS) $(LINKL_CFLAGS) $< -o $@ 

clean:
	rm -f *.o	

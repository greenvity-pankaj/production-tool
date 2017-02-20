GREEN_CFLAGS = -I./ -I./hal/ -I./link/ -I./mux/ -I./ctrl/ -I./nma/ -I./../../common 

ifeq ($(CONFIG_HPGP_HAL),y)
        GREEN_CFLAGS += -I./../../hal
endif

ifeq ($(SIMULATE),y)
        GREEN_CFLAGS += -I./simu/
        GREEN_CFLAGS += -I./test/
endif

GREEN_OBJS = green.o hpgpctrl.o

#PAPOBJS = $(addprefix ./obj/, $(PAP_OBJS))

all: $(GREEN_OBJS)

$(GREEN_OBJS):%.o:%.c 
	$(CC) $(CFLAGS) $(GREEN_CFLAGS) -c $< -o $@ \


clean:
	rm -f *.o

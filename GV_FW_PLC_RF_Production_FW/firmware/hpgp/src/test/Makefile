HTEST_CFLAGS = -I./ -I./.. -I./../hal/ -I./../link/ -I./../mux/ -I./../ctrl/ -I./../../../common -I./../nma/ -I./../crypto/

HTEST_OBJS = host.o uim.o nmm.o nmmh1.o
ifeq ($(CONFIG_HPGP_TEST),y)
HTEST_OBJS += htm.o
endif

#PAPOBJS = $(addprefix ./obj/, $(PAP_OBJS))

all: $(HTEST_OBJS)

$(HTEST_OBJS):%.o:%.c 
	$(CC) $(CFLAGS) $(HTEST_CFLAGS) -c $< -o $@ \


clean:
	rm -f *.o

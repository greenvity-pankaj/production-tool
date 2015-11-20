PAP_CFLAGS = -I./ -I ./../hpgp/src/ -I./../hpgp/src/hal/ -I./../hpgp/src/link/ \
		-I./../hpgp/src/mux/ -I./../hpgp/src/ctrl/ -I./../hal

PAP_OBJS = fm.o event.o stm.o ism.o sched.o list.o

#PAPOBJS = $(addprefix ./obj/, $(PAP_OBJS))

all: $(PAP_OBJS)

$(PAP_OBJS):%.o:%.c 
	$(CC) $(CFLAGS) $(PAP_CFLAGS) -c $< -o $@ \


clean:
	rm -f *.o

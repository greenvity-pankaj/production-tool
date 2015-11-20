
CTRLL_CFLAGS += -I./ -I./../ -I./../../../common -I./../link -I./../nma -I./../mux -I./../hal

CTRLL_OBJS = ctrll.o 
	 

all: $(CTRLL_OBJS)

$(CTRLL_OBJS):%.o: %.c
	$(CC) -c $(CFLAGS) $(CTRLL_CFLAGS) $< -o $@ 

clean:
	rm -f *.o	

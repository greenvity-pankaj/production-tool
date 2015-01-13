
NMA_CFLAGS += -I./ -I./../ -I./../../../common/ -I./../test/ -I./../ctrl/ -I./../link -I./../mux -I./../hal

NMA_OBJS = nma.o nmah1.o

all: $(NMA_OBJS)

$(NMA_OBJS):%.o: %.c
	$(CC) -c $(CFLAGS) $(NMA_CFLAGS) $< -o $@ 

clean:
	rm -f *.o	

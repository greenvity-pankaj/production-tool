
ENC_CFLAGS += -I./ -I./../../../common

ENC_OBJS = sha2.o pbkdf1.o crc32.o

ifeq ($(SIMULATE),y)
        ENC_OBJS += aes.o
endif


all: $(ENC_OBJS)

$(ENC_OBJS):%.o: %.c
	$(CC) -c $(CFLAGS) $(ENC_CFLAGS) $< -o $@ 

clean:
	rm -f *.o	

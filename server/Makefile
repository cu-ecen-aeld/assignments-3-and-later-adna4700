	
CC := $(CROSS-COMPILE)gcc
CFLAGS := -g -Wall -Werror
SRC := aesdsocket.c
TARGET := aesdsocket
LDFLAGS := -pthread -lrt


all: $(TARGET)
	 $(CROSS_COMPILE)$(CC) $(CFLAGS) ${SRC} -o ${TARGET} ${LDFLAGS}

clean:
	rm -f *.o $(TARGET) *.elf *.map
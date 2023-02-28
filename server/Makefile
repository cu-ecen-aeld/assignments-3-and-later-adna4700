	
CC := $(CROSS-COMPILE)gcc
CFLAGS := -g -Wall -Werror
SRC := aesdsocket.c
TARGET := aesdsocket


all: $(TARGET)
	 $(CROSS_COMPILE)$(CC) $(CFLAGS) ${SRC} -o ${TARGET}

clean:
	rm -f *.o $(TARGET) *.elf *.map
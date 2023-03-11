#Assignment 2: AESD:- Intro to system programming
#Author: Aditi Nanaware
#This is a make file
#Resources: https://stackoverflow.com/questions/70824666/how-to-cross-compile-for-arm-on-ubuntu-#conditionally

CC = $(CROSS_COMPILE)gcc

#A default target which builds the “writer” application
default: writer
all: writer
writer: writer.o
	${CC} -Wall writer.c -o writer
	
#A clean target which removes the “writer” application and all .o files
clean:
	rm *.o writer

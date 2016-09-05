cc = g++
cppflag = -std=c++11 -Wextra -Wall 
LFLAGS = -std=c++11 -Wall -g

all: libuthreads.a

libuthreads.a: uthreads.o
	ar -rcs libuthreads.a uthreads.o

uthreads.o: uthreads.cpp uthreads.h
	$(cc) -c $(cppflag) uthreads.cpp


clean:
	rm -f *.o MAIN

.PHONY: all tar clean

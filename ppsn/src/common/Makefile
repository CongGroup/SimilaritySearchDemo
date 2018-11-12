CC=g++
GCC=gcc
DEST_LIB=libcommon.a
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o) oi_shm.o
INC_FILE=cbase_ctl.h cpkg_adapt.h ce2lsh.h oi_shm.h
INC=-I../inc
FLAGS=-Wall -g -std=c++0x -O2
RM=-rm -f

.PHONY: all clean

all: $(DEST_LIB)

-include $(SRC:.cpp=.d)

%.d: %.cpp
	$(CC) -MM $(FLAGS) $(INC) $< -o $@

%.o: %.cpp %.d
	$(CC) -c $(FLAGS) $(INC) $< -o $@

oi_shm.o:
	$(GCC) -c $(FLAGS) $(INC) oi_shm.c -o oi_shm.o

$(DEST_LIB): $(OBJ)
	ar cqs $(DEST_LIB) $^
	cp $(DEST_LIB) ../lib
	
clean:
	$(RM) *.o
	$(RM) *.d
	$(RM) $(DEST_LIB)
	$(RM) ../lib/$(DEST_LIB)

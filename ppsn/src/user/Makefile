CC=g++
DEST_LIB=libuser.a
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)
INC_FILE=cbase_ctl.h cimg_ctl.h
INC=-I../inc
OPENCV_LIB=`pkg-config opencv --libs`
OPENCV_INC=`pkg-config opencv --cflags`
FLAGS=-Wall -g $(OPENCV) -O2
RM=-rm -f

.PHONY: all clean

all: $(DEST_LIB)

-include $(SRC:.cpp=.d)

%.d: %.cpp
	$(CC) -MM $(FLAGS) $(OPENCV_INC) $(INC) $< -o $@

%.o: %.cpp %.d
	$(CC) -c $(FLAGS) $(OPENCV_INC) $(INC) $< -o $@

$(DEST_LIB): $(OBJ)
	ar cqs $(DEST_LIB) $^
	cp $(DEST_LIB) ../lib
	
clean:
	$(RM) *.o
	$(RM) *.d
	$(RM) $(DEST_LIB)
	$(RM) ../lib/$(DEST_LIB)

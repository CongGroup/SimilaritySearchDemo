CC=g++
BIN=user_client
FLAGS=-Wall -g -std=c++0x -fpermissive -O2
RM=-rm -f
SRC=$(wildcard *.cpp)

#islib configuration
#ISLIB_LINK_MODULE=neocomm
#ISLIB_VERSION=v1
#ISLIB_BASE=~/islib
#include $(ISLIB_BASE)/tags/demo/islib.mk

#SRC LIB & INC
SRC_INC=-I../inc
SRC_LIB=-L../lib

#OPENCV LIB & INC
OPENCV_LIB=`pkg-config opencv --libs`
OPENCV_INC=`pkg-config opencv --cflags`

#common
COMMON_LINK=-lcommon

#lib user
USER_LINK=-luser

#boost lib
BOOST_INC=-I/usr/local/include
BOOST_LIB=-L/usr/local/lib
BOOST_LINK=-lboost_system -lboost_filesystem

#openssl
OPENSSL_LINK=-lcrypto
OPENSSL_INC=-I/usr/local/ssl/include
OPENSSL_LIB=-L/usr/local/ssl/lib

#combine the inc & lib
INC=$(ISLIB_INC) $(SRC_INC) $(OPENCV_INC) $(BOOST_INC) $(OPENSSL_INC)
LIB=$(ISLIB_LIB) $(ISLIB_LINK) $(SRC_LIB) $(COMMON_LINK) $(BOOST_LIB) $(OPENSSL_LIB) $(OPENSSL_LINK)

all: $(BIN)

%.d: %.cpp
	$(CC) -MM $(FLAGS) $(INC) $< -o $@

%.o: %.cpp %.d
	$(CC) -c $(FLAGS) $(INC) $< -o $@

user_client:user_client.o
	$(CC) $(FLAGS) -o $@ $@.o $(OBJ) $(USER_LINK) $(OPENCV_LIB) $(LIB) $(BOOST_LINK)

clean:
	$(RM) *.o
	$(RM) *.d
	$(RM) $(BIN)

-include $(SRC:.cpp=.d)

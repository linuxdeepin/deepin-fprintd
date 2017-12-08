CC = gcc
TEST = op_test

PWD = $(shell pwd)
SRC_DIR := ${PWD}
vpath %.c ${SRC_DIR}

CFLAGS = -Wall -g `pkg-config --cflags libfprint`
LDFLAGS = `pkg-config --libs libfprint`

COMMON_OBJS = device.o storage.o utils.o 
TEST_OBJS := ${COMMON_OBJS} _op_test.o

all : ${TEST}

${TEST}: ${TEST_OBJS}
	${CC} ${LDFLAGS} $^ -o $@

%.o : %.c
	${CC} ${CFLAGS} -c $<

clean :
	rm -f ${TEST_OBJS} ${TEST}

rebuild : clean all

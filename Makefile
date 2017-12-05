CC = gcc
TARGET = deepin-fprintd

SRC_DIR := src/
vpath %.c ${SRC_DIR}

CFLAGS = -Wall -g `pkg-config --cflags libfprint glib-2.0`
LDFLAGS = `pkg-config --libs libfprint glib-2.0`

OBJS := device.o storage.o utils.o main.o

all : ${TARGET}

${TARGET} : ${OBJS}
	${CC} ${LDFLAGS} $^ -o $@

%.o : %.c
	${CC} ${CFLAGS} -c $<

clean :
	rm -f ${OBJS} ${TARGET}

rebuild : clean all

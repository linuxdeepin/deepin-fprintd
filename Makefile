CC = gcc
TARGET = deepin-fprintd
OPERATION_TEST = operation_test

SRC_DIR := src/
vpath %.c ${SRC_DIR}

CFLAGS = -Wall -g `pkg-config --cflags libfprint`
LDFLAGS = `pkg-config --libs libfprint`

COMMON_OBJS = device.o storage.o utils.o 
OPTEST_OBJS := ${COMMON_OBJS} _operation_test.o

all : ${TARGET}

${TARGET} : 
	cd src && go build -o ../$@

${OPERATION_TEST}: ${OPTEST_OBJS}
	${CC} ${LDFLAGS} $^ -o $@

%.o : %.c
	${CC} ${CFLAGS} -c $<

clean :
	rm -f ${OBJS} ${TARGET} ${OPTEST_OBJS} ${OPERATION_TEST}

install:
	mkdir -p /usr/bin
	cp ${TARGET} /usr/bin/

	mkdir -p /usr/share/dbus-1/system.d
	cp data/dbus/system.d/com.deepin.daemon.Fprintd.conf /usr/share/dbus-1/system.d/
	mkdir /usr/share/dbus-1/system-services
	cp data/dbus/system-services/com.deepin.daemon.Fprintd.service /usr/share/dbus-1/system-services/

rebuild : clean all

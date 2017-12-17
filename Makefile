PREFIX = /usr
CC = gcc
TARGET = deepin-fprintd
PAM = pam_deepin_fprintd.so
TEST = op_test

SRC_DIR := src/
vpath %.c ${SRC_DIR}

CFLAGS = -Wall -g `pkg-config --cflags libfprint`
LDFLAGS = `pkg-config --libs libfprint`

COMMON_SRCS := src/device.c src/storage.c src/utils.c
PAM_SRCS := ${COMMON_SRCS} pam/pam.c

COMMON_OBJS = device.o storage.o utils.o
TEST_OBJS := ${COMMON_OBJS} _op_test.o

all : ${TARGET} ${PAM}

${TARGET} :
	cd src && go build -o ../$@ && cd ../

${PAM} : ${PAM_SRCS}
	${CC} ${LDFLAGS} -fPIC -shared $^ -o $@

${TEST}: ${TEST_OBJS}
	${CC} ${LDFLAGS} $^ -o $@

%.o : %.c
	${CC} ${CFLAGS} -c $<

clean :
	rm -f ${OBJS} ${TARGET} ${TEST_OBJS} ${TEST} ${PAM}

install:
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${TARGET} ${DESTDIR}${PREFIX}/bin/

	mkdir -p ${DESTDIR}${PREFIX}/lib/x86_64-linux-gnu/security/
	cp -f ${PAM} ${DESTDIR}${PREFIX}/lib/x86_64-linux-gnu/security/

	mkdir -p ${DESTDIR}${PREFIX}/share/dbus-1/system.d
	cp -f data/dbus-1/system.d/com.deepin.daemon.Fprintd.conf ${DESTDIR}${PREFIX}/share/dbus-1/system.d/
	mkdir -p ${DESTDIR}${PREFIX}/share/dbus-1/system-services
	cp -f data/dbus-1/system-services/com.deepin.daemon.Fprintd.service ${DESTDIR}${PREFIX}/share/dbus-1/system-services/

rebuild : clean all

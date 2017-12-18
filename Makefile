PREFIX = /usr
CC = gcc
TARGET = deepin-fprintd
PAM = pam_deepin_fprintd.so
TEST = op_test

ARCH = $(shell getconf LONG_BIT)
ifeq (64,$(ARCH))
LIB_INSTALL_DIR = /usr/lib/x86_64-linux-gnu
else
LIB_INSTALL_DIR = /usr/lib/i386-linux-gnu
endif

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

	mkdir -p ${DESTDIR}${PREFIX}/lib/$(LIB_INSTALL_DIR)/security/
	cp -f ${PAM} ${DESTDIR}${PREFIX}/lib/$(LIB_INSTALL_DIR)/security/

	mkdir -p ${DESTDIR}${PREFIX}/share/dbus-1/system.d
	cp -f data/dbus-1/system.d/com.deepin.daemon.Fprintd.conf ${DESTDIR}${PREFIX}/share/dbus-1/system.d/
	mkdir -p ${DESTDIR}${PREFIX}/share/dbus-1/system-services
	cp -f data/dbus-1/system-services/com.deepin.daemon.Fprintd.service ${DESTDIR}${PREFIX}/share/dbus-1/system-services/

rebuild : clean all

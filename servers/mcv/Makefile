#Makefile for mcv server
PROG = mcv
SRCS = main.c queue.c

DPADD+= ${LIBSYS}
LDADD+= -lsys

MAN=

BINDIR?= /usr/sbin
FILES=mcv.conf
FILESNAME=mcv
FILESDIR= /etc/system.conf.d

.include <minix.service.mk>


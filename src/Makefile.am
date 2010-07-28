AM_CFLAGS = -I./ -std=gnu99 -Wall -pedantic -Werror -pthread
bin_PROGRAMS = soto

soto_SOURCES = alsagw.c headers/alsagw.h \
               rtutils.c headers/rtutils.h \
               thrd.c headers/thrd.h \
               sampthread.c headers/sampthread.h \
               headers/logging.h \
               headers/constants.h \
               main.c

soto_LDADD = -lasound -ldacav -lrt -lthdacav -lplot #-lefence


AM_CFLAGS = -I./ -std=gnu99 -Wall -Werror -pthread
bin_PROGRAMS = soto

soto_SOURCES = main.c \
               alsagw.c headers/alsagw.h \
               rtutils.c headers/rtutils.h \
               sampthread.c headers/sampthread.h \
               options.c headers/options.h \
               dispatch.c headers/dispatch.h \
               thrd.c headers/thrd.h \
               plotthread.c headers/plotthread.h \
               plotting.c headers/plotting.h \
               headers/logging.h

soto_LDADD = -lasound -ldacav -lrt -lthdacav -lplot #-lefence


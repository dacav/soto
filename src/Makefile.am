AM_CFLAGS = -I./ -std=gnu99 -Wall -Werror -pthread
bin_PROGRAMS = soto

soto_SOURCES = main.c \
               alsasample.c headers/alsasample.h \
			   rtutils.c headers/rtutils.h \
			   sampthread.c headers/sampthread.h \
               options.c headers/options.h \
               thrd.c headers/thrd.h \
               headers/logging.h

soto_LDADD = -lasound -ldacav -lrt -lthdacav


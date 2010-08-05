AM_CFLAGS = -I./ -std=gnu99 -Wall -pedantic -Werror -pthread
bin_PROGRAMS = soto

soto_SOURCES = alsagw.c headers/alsagw.h \
               rtutils.c headers/rtutils.h \
               thrd.c headers/thrd.h \
               plotting.c headers/plotting.h \
			   options.c headers/options.h \
               genthrd.c headers/genthrd.h \
               plotthread.c headers/plotthread.h \
               signal_show.c headers/signal_show.h \
               spectrum_show.c headers/spectrum_show.h \
               sampthread.c headers/sampthread.h \
               headers/logging.h \
               headers/constants.h \
               main.c

soto_LDADD = -lasound -ldacav -lrt -lplot -lfftw3


AM_CFLAGS = -I./ -std=gnu99 -Wall -Werror -pthread
bin_PROGRAMS = soto

soto_SOURCES = main.c \
               options.c headers/options.h \
               thrd.c headers/thrd.h \
               headers/logging.h
soto_LDADD = -lasound -ldacav


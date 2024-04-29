SOURCES:=$(wildcard *.c )
OBJECTS:=$(SOURCES:.c=.o)
CFLAGS:=-I. -O3 -g0 -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200000L
LIBS:=-lpng -lm -lrt 
all: imgdiff

$(OBJECTS): Makefile

imgdiff: $(OBJECTS)
	${CC} -o $@ $^ ${LIBS}

clean:
	rm -f ${OBJECTS} imgdiff  *.*~

indent: uncrustify.cfg
	uncrustify -c uncrustify.cfg --replace $(SOURCES)

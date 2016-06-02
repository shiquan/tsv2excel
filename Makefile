CC=		gcc
CFLAGS= -Wall
PROG=		tsv2excel
INCLUDES=	-I.
SUBDIRS=	. 
LIBPATH=        -L.

.SUFFIXES:.c .o
.PHONY: all

.c.o:
		$(CC) -c $(CFLAGS) $(DFLAGS) $(INCLUDES) $< -o $@

all:clean $(PROG) 

.PHONY:all  clean

tsv2excel:
		$(CC) $(CFLAGS) $(LDFLAGS) $(LIBPATH) $(INCLUDES) -lxlsxwriter -lz -o tsv2excel tsv2excel.c kstring.c

debug:
		$(CC) -g -O0 $(CFLAGS) $(LDFLAGS) $(LIBPATH) $(INCLUDES) -D_DEBUG_MODE -lxlsxwriter -lz -o tsv2excel tsv2excel.c kstring.c

clean:
		rm -fr gmon.out *.o a.out *.exe *.dSYM  $(PROG) *~ *.a 

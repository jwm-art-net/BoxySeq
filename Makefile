PREFIX :=/usr/local
BINDIR :=$(PREFIX)/bin


PROG := boxyseq


CC := gcc

DEFS := -std=gnu99 -fsigned-char -Wall -Wextra 			\
	-Wunused-parameter -Wfloat-equal -Wshadow 		\
	-Wunsafe-loop-optimizations 				\
	 -funsafe-loop-optimizations 				\
	-Wpointer-arith -Wbad-function-cast 			\
	-Wcast-qual -Wcast-align -Wwrite-strings 		\
	-Wconversion -Waddress -Wlogical-op -Wmissing-prototypes\
	-Wmissing-declarations -Winline

HDRS := `pkg-config --cflags gtk+-2.0` `pkg-config --cflags jack`

DEBUGS:=-DFREESPACE_DEBUG 	\
	-DEVPOOL_DEBUG		

#	-DGRID_DEBUG		
#	-DNO_REAL_TIME
#	-DLLIST_DEBUG
#	-DPATTERN_DEBUG		\


DEFINES:= -DUSE_64BIT_ARRAY $(DEBUGS)

CFLAGS := -ggdb -O0 $(DEFINES)

#CFLAGS := $(DEFINES) -march=core2 -O3 -pipe -fomit-frame-pointer -Winline

LIBS   := `pkg-config --libs gtk+-2.0` `pkg-config --libs jack`

SRC    := $(wildcard *.c)
OBJS   := $(patsubst %.c, %.o, $(SRC))
HEADERS:= $(wildcard *.h)

$(PROG): $(OBJS)
	$(CC) $(DEFS) $(HDRS) $(CFLAGS) $(OBJS) -o $(PROG) $(LIBS)
	
%.o : %.c $(HEADERS)
	$(CC) $(DEFS) $(HDRS) $(CFLAGS) $(LIBS) -c $<

main.o: main.c $(HEADERS)

clean:
	rm -f $(PROG) $(OBJS)

install: $(PROG)
	install $(PROG) $(BINDIR)


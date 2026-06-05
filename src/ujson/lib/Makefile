PROGRAM=libujson.a
CPP=gcc
LIBS=-lm
SOURCE=.
OBJS=ultrajsonenc.o ultrajsondec.o
LINKFLAGS=-shared
#-Wl,-soname,libultrajson.so.1

CPPFLAGS=-D_REENTRANT -D_LINUX
all : CPPFLAGS += -O3 -DNDEBUG -fPIC
all : libultrajson

libultrajson : $(OBJS)
	ar rcs ./$(PROGRAM) $(OBJS)
	cp ./$(PROGRAM) ./python/lib/

%.o:    $(SOURCE)/%.c
	$(CPP) $(CPPFLAGS) -c $< -o ./$@

clean:
	rm -rf *.o
	rm -rf $(PROGRAM)

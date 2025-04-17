CC = gcc
LIBS = -L. -L./bin/
SRC = src/
INC = include/
CFLAGS = -Wall -std=c11
LIBSRC = bin/

test: test.o $(LIBSRC)libvcparser.so
	$(CC) $(CFLAGS) $(LIBS) -o  test test.o -lvcparser

test.o: $(SRC)test.c $(INC)VCParser.h
	$(CC) $(CFLAGS) -I$(INC) -c $(SRC)test.c

parser: VCParser.o VCHelper.o LinkedListAPI.o
	$(CC) -shared -o $(LIBSRC)libvcparser.so VCParser.o VCHelper.o LinkedListAPI.o

VCParser.o: $(SRC)VCParser.c $(INC)VCParser.h $(INC)VCHelper.h
	$(CC) -I$(INC) $(CFLAGS) -c -fpic $(SRC)VCParser.c

VCHelper.o: $(SRC)VCHelper.c $(INC)VCHelper.h
	$(CC) -I$(INC) $(CFLAGS) -c -fpic $(SRC)VCHelper.c

LinkedListAPI.o: $(SRC)LinkedListAPI.c $(INC)LinkedListAPI.h
	$(CC) -I$(INC) $(CFLAGS) -c -fpic $(SRC)LinkedListAPI.c

clean:
	rm *.o $(LIBSRC)*.so
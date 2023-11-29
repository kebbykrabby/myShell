CC=gcc
CFLAGS=-g -Wall  -m32
OBJS=myshell.c LineParser.c 

all: myshell

myshell: $(OBJS)
	gcc -g -Wall -c LineParser.c -o LineParser.o
	gcc -g -Wall -c myshell.c -o myshell.o
	gcc -g -Wall  LineParser.o myshell.o -o myshell

mypipe: mypipe.c 
	gcc -g -Wall -c mypipe.c -o mypipe.o
	gcc -g -Wall -c mypipe.c -o mypipe

myshell.o: myshell.c
	$(CC) $(CFLAGS) -c myshell.c -o myshell.o

LineParser.o: LineParser.c
	$(CC) $(CFLAGS) -c LineParser.c -o LineParser.o



clean:
	rm -f ./*.o myshell mypipe
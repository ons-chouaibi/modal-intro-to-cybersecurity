all: file
file : file.o
	cc -g -o file file.o
file.o: file.c
	cc -g -Wall -c file.c
clean:
	rm -f file file.o
all : server

server : main.o
	cc -o server main.o

main.o : main.c config.h
	cc -c main.c

clean :
	rm main.o

all : server

server : main.o server.o
	cc -o server main.o server.o

main.o : main.c config.h
	cc -c main.c

server.o : server.c
	cc -c server.c

clean :
	rm main.o server.o

FLAGS = -std=c99

all : server

server : main.o server.o
	cc $(FLAGS) -o server main.o server.o

main.o : main.c config.h
	cc $(FLAGS) -c main.c

server.o : server.c
	cc $(FLAGS) -c server.c

clean :
	-rm main.o server.o server

FLAGS = -std=c99

all : telnet-server

telnet-server : main.o server.o
	cc $(FLAGS) -o telnet-server main.o server.o

main.o : main.c config.h
	cc $(FLAGS) -c main.c

server.o : server.c
	cc $(FLAGS) -c server.c

clean :
	-rm main.o server.o telnet-server


CC=gcc
CFLAGS=-g $(ENVCFLAGS) -Wall -W -Wno-unused-function \
       -Wno-unused-parameter #-DDEBUG
LIBS=$(ENVLIBS)
MAKEFILE=Makefile
LN=ln
RM=rm
AR=ar crus

all: server client

server: server.c
	$(CC) $(CFLAGS) server.c -o server 
client: clients.c
	$(CC) $(CFLAGS) clients.c -o client

clean: 
	rm server client
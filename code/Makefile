SHELL := /bin/bash
CC = gcc

CFLAGS = -pthread -g -std=c99

BINS = bin
OBJ = obj
SRC = src
HEADDIR = includes

OBJC = obj/api.o obj/comms.o obj/optparsing.o obj/storage.o obj/icl_hash.o
OBJS = obj/storage.o obj/requestqueue.o obj/comms.o obj/configparsing.o obj/icl_hash.o

.PHONY: all clean cleanall test1 test2 test3

all: clean server client

server: $(SRC)/$< $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(SRC)/$@.c -o $(BINS)/$@

client: $(SRC)/$< $(OBJC)
	$(CC) $(CFLAGS) $(OBJC) $(SRC)/$@.c -o $(BINS)/$@

test1: clean server client
	mkdir -p test1savedfiles/1
	mkdir -p test1savedfiles/2
	bash ./scripts/test1.sh
	bash ./statistiche.sh log.txt

test2: clean server client
	mkdir -p test2savedfiles/evicted
	mkdir -p test2savedfiles/evicted2
	bash ./scripts/test2.sh
	bash ./statistiche.sh log.txt

test3: clean server client
	mkdir -p test3savedfiles/1
	mkdir -p test3savedfiles/2
	mkdir -p test3savedfiles/3
	bash ./scripts/test3.sh
	bash ./statistiche.sh log.txt

$(OBJ)/%.o: $(SRC)/%.c $(HEADDIR)/%.h
	$(CC) -c $(CFLAGS) $< -o $@ 

clean:
	rm -f $(OBJ)/*.o
	rm -r -f $(BINS)/*
	rm -f *.sk
	rm -f saved/* # REMOVE THIS AFTER TESTING
	rm -f evicted/* 

cleanall :
	rm -f $(OBJ)/*.o
	rm -r -f $(BINS)/*
	rm -f log.txt
	rm -f *.sk
	rm -r -f test1savedfiles
	rm -r -f test2savedfiles
	rm -r -f test3savedfiles
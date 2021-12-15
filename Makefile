CFLAGS = -I ./include -pthread
CC = gcc
 
TARGET  = app
BIN_DIR = ./bin
SRC_DIR = ./src

start: consumer saler producer
	mv $^ $(BIN_DIR)

consumer: $(SRC_DIR)/consumer.c
	gcc -o $@ $< $(CFLAGS) -lrt

saler: $(SRC_DIR)/saler.c
	gcc -o $@ $< $(CFLAGS) -lrt

producer: $(SRC_DIR)/producer.c
	gcc -o $@ $< $(CFLAGS)




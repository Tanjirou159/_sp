.PHONY: all bank producer_consumer dining_philosophers clean
CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGETS = bank producer_consumer dining_philosophers

all: $(TARGETS)

bank: bank.c
	$(CC) $(CFLAGS) -o bank bank.c

producer_consumer: producer_consumer.c
	$(CC) $(CFLAGS) -o producer_consumer producer_consumer.c

dining_philosophers: dining_philosophers.c
	$(CC) $(CFLAGS) -o dining_philosophers dining_philosophers.c

clean:
	rm -f $(TARGETS) *.o

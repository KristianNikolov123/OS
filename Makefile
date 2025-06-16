CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -lrt

all: producer consumer

producer: producer.c common.h
	$(CC) $(CFLAGS) -o producer producer.c $(LDFLAGS)

consumer: consumer.c common.h
	$(CC) $(CFLAGS) -o consumer consumer.c $(LDFLAGS)

clean:
	rm -f producer consumer 
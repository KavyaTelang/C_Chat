CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGETS = server client

all: $(TARGETS)

server: chat_server.c
	$(CC) $(CFLAGS) chat_server.c -o server

client: chat_client.c
	$(CC) $(CFLAGS) chat_client.c -o client

clean:
	rm -f $(TARGETS)

.PHONY: all clean
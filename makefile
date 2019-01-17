all: server client
server: server.c
	gcc	-o	server	server.c -lsqlite3 -pthread
client: client.c	
	gcc	-o	client	client.c -lsqlite3 -pthread
clean:
	find . -type f | xargs touch
	rm -rf $(OBJS)	
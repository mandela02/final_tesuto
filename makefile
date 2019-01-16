all: server client
server: server.c
	gcc	-o	server	server.c -lsqlite3
client: client.c	
	gcc	-o	client	client.c -lsqlite3
clean:
	find . -type f | xargs touch
	rm -rf $(OBJS)	
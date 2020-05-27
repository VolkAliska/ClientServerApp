TARGET=client server
CC=g++
normal: $(TARGET)
client: client.cpp
	$(CC) client.cpp -o client
server: server.cpp
	$(CC) server.cpp -o myserver
clean:
	$(RM) $(TARGET)
stopserver:
	./stopserver.sh myserver
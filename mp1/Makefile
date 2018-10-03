all:
	make client server
client: http_client.o
	g++ -o http_client http_client.o
server: http_server.o
	g++ -o http_server http_server.o
http_client.o: http_client.cpp http_client.h
	g++ -c -g http_client.cpp
http_server.o: http_server.cpp
	g++ -c -g http_server.cpp
clean:
	rm -f core http_client http_server *.o
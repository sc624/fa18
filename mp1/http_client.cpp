/*
** http_client.cpp -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <fstream>

#include "http_client.h"
#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 
#define DEFAULT_PORT "80"
#define TEN_MB 1048576 // max number of bytes we can get at once 
using namespace std;


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


//parse args
request_args parse_args(char * argv[]){
	string http = "http://";
	string file_arg, host_arg = "";
	string port_num = DEFAULT_PORT;
	string argument = string(argv[1]);
	request_args output;
	if (argument.compare(0, 7, http)){
		fprintf(stderr, "http:// missing");
		exit(1);
	}
	int s_pos = argument.find("/", 7);
	int c_pos = argument.find(":", 7);
	int h_pos = 0;
	if (s_pos > 0){
		file_arg.append(argument.substr(s_pos, argument.length()));
		if (c_pos > 7){
			port_num = argument.substr(c_pos + 1, s_pos - c_pos - 1).c_str();
			h_pos = c_pos;
		}
		else{
			h_pos = s_pos;
		}
	}
	else{
		fprintf(stderr, "no file");
		exit(1);
	}

	host_arg.append(argument.substr(7, h_pos - 7));
	output.file = file_arg;
	output.host = host_arg;
	output.port = port_num;
	return output;

}



int main(int argc, char *argv[]){
	int sockfd, numbytes;  
	char buffer[TEN_MB];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	string request = "";
	string port = "80";


	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//parse command line args
	request_args http_args = parse_args(argv);
    request.append("GET " + http_args.file + " HTTP/1.1\r\n");
    request.append("Host: " + http_args.host + ":" + http_args.port+"\r\n");
    request.append("\r\n");
    cout << request << endl;

    rv = getaddrinfo(http_args.host.c_str(), http_args.port.c_str(), &hints, &servinfo);
	if ( rv != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	if (send(sockfd, request.c_str(), request.length(), 0) != request.length()){
		cout << "send Error" << endl;
		return -1;
	}

	string response= "";
	while(1) {
		if ((numbytes = recv(sockfd, buffer, TEN_MB-1, 0)) > 0) {
	    		response.append(buffer, numbytes);
        }
		else{
			break;
		}
	}
	close(sockfd);

	int first = response.find(" ");
	if (first > 0){
		int second = response.find(" ", first + 1);
		response.append(response.substr(first + 1, second - first - 1));
	}
	else {
		cout << "Invalid response" << endl;
		return -1;
	}

	
	if (response == "400"){
		cout << "Error 400 - Bad request" << endl;
		return -1;
	}
	if (response == "404"){
        cout << "Error 404 - File not found" << endl;
        return -1;
    }	


	int findex = response.find("\r\n\r\n");
	if (findex > 0){
		findex += 4;
        if ((response[findex] == '3') && (response[findex+1] == 'e') && (response[findex+2]=='8')&&(response[findex+3]=='0')){
            findex += 6;
        }
    }
	else {
		cout << "client: File returned is invalid" << endl;
		return -1;
	}

	ofstream file;
	file.open("output");
	file << response.substr(findex, response.length() - findex);
	file.close();


	return 0;
}

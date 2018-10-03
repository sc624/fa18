/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "http_server.h"

#include <sys/time.h>
#include <unistd.h>

#define PORT "3490"  // the port users will be connecting to
#define TEN_KB 1000 // max number of bytes we can get at once 
#define BACKLOG 20	 // how many pending connections queue will hold


void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


//FROM BEEJ'S GUIDE 
int sendall(int s, const char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 500000;
    fd_set writefds;
	FD_SET(s, &writefds);



    while(total < *len) {
        select(s+1, NULL, &writefds, NULL, &tv);
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

string output(request_t request, string status_code, string file){
	//finds status code of response
	string response = "";
	string status = "";
	if (status_code == "200"){
		status = "OK";
	}
	else if (status_code == "404"){
		status = "Not found";
	}
	else {
		status = "Bad request";
	}
	

	//gets current time and date
	time_t current = time(0);
	string date = ctime(&current);
	if (!date.empty() && date[date.length()-1] == '\n') {
    		date.erase(date.length()-1);
	}
  
	string date_label = "Date: " + date;

	//
	if (status_code == "404" || status_code == "400"){
		response.append(request.protocol +" "+status_code+" "+status+"\r\n");
		response.append("Content-Length: 0\r\n");
		response.append("Content-Type: text/plain\r\n\r\n");
	}
	else {
		int type = request.file.find(".");
		string file_type = request.file.substr(type + 1);
		// form header
		response.append(request.protocol+" "+status_code+" "+status+"\r\n");
		response.append(date + "\r\n");
		response.append("Content-Type: text/plain\r\n\r\n");

		// form document
		response.append(file);
	}
	return response;
}



int main(int argc, char * argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv, idx, idx_1, idx_2, idx_3, numbytes;
	char buffer[TEN_KB];


	if(argc != 2){
		cout << "Port number # needed" << endl;
		exit(1);
	}
	
	char * PORT_num = argv[1];
	cout << "server will use port #: " << PORT_num << endl;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	rv = getaddrinfo(NULL, PORT_num, &hints, &servinfo);
	if ( rv != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connection\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			if ((numbytes = recv(new_fd, buffer, TEN_KB-1, 0))== -1){
				perror("recv");
			}

			buffer[numbytes] = '\0';
			string arg_buffer = string(buffer);
			request_t buddy;		
			
			//request file_type
			buddy.first = arg_buffer.find(" ");
		    if (buddy.first == -1){
		        arg_buffer = "-1";
		    }
			buddy.type = arg_buffer.substr(0, buddy.first);
			if (!(buddy.type == "GET" || buddy.type == "POST")){
				buddy.type = "-1";
			}
			cout << "request type: " << buddy.type << endl;

			//file name
			buddy.second = arg_buffer.find(" ", buddy.first + 1);
			buddy.file = arg_buffer.substr(buddy.first + 2, buddy.second - buddy.first - 2);
			cout << "filename: " << buddy.file << endl;

			//protocol
			idx = arg_buffer.find("\n");
			buddy.protocol = arg_buffer.substr(buddy.second + 1, idx - buddy.second - 2);
			cout << "http protocol: " << buddy.protocol << endl;

			//user
			idx_2 = arg_buffer.find("User-Agent: ");
			if (idx_2 == -1)
				buddy.user_agent = "";
			idx_1 = arg_buffer.find(" ", idx_2);
			idx_3 = arg_buffer.find("\n", idx_1 + 1);
			buddy.user_agent = arg_buffer.substr(idx_1 + 1, idx_3 - idx_1 - 1);

			//host
			idx_2 = arg_buffer.find("Host: ");
			if (idx_2 == -1)
				buddy.host = "";
			idx_1 = arg_buffer.find(" ", idx_2);
			idx_3 = arg_buffer.find(":", idx_1 + 1);
			buddy.host = arg_buffer.substr(idx_1 + 1, idx_3 - idx_1 - 1);

			//accept
			idx_2 = arg_buffer.find("Accept: ");
			if (idx_2 == -1)
				buddy.accept = "";
			idx_1 = arg_buffer.find(" ", idx_2);
			idx_3 = arg_buffer.find("\n", idx_1 + 1);
			buddy.accept = arg_buffer.substr(idx_1 + 1, idx_3 - idx_1 - 1);

			//accept-encoding
			idx_2 = arg_buffer.find("Accept-Encoding: ");
			if (idx_2 == -1)
				buddy.accept_encoding = "";
			idx_1 = arg_buffer.find(" ", idx_2);
			idx_3 = arg_buffer.find("\n", idx_1 + 1);
			buddy.accept_encoding = arg_buffer.substr(idx_1 + 1, idx_3 - idx_1 - 1);



			//status messages
			string file_string, code = "";
			string path = buddy.file;
			ifstream req_file(path.c_str());

			// change status_code to 400 for bad request
			// check request->filename, host, etc.

			if (buddy.type == "-1"){
				code = "400";
			}
			else if (!req_file){
				code = "404";
			}
			else {
				stringstream filestream;
				filestream << req_file.rdbuf();
				file_string = filestream.str();
				code = "200";
			}


			string response = output(buddy, code, file_string);

			int size = response.size();



			if (sendall(new_fd, response.c_str(), &size) == -1)
				perror("send error");
			close(new_fd);
			exit(0);


		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}


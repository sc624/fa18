#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H


#include <iostream>
#include <string>
#include <sstream> 
#include <fstream>
#include <streambuf>


using namespace std;
struct request_t{
	string type, file, protocol, user_agent, host, accept, accept_encoding;
	int first, second;
};

#endif

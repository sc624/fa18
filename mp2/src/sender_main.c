#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>


#define BUFSIZE 2000
#define HEADER_SIZE 50
#define WINDOW 400
#define SLOW_GROWTH 8
#define SLOW_DECLINE 8
#define PACKET_SIZE 1472



/** Window variables */

struct addrinfo *p;

void diep(char *s) {
    perror(s);
    exit(1);
}


void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer)
{
    struct sockaddr_in si_other;
    socklen_t slen = sizeof(si_other);
    int s, rtt;
    int counter = 0;

    int lastIndex = -1/*, lastBytes = -1*/;
    double slowGrowth = 0.0;
    long long int totalRead = 0, bytes_read = 0;

    unsigned int expectedAck = 0, frameCount = 0;
    // unsigned long long totalBytes = 0;

    int fileRead[WINDOW];
    int size_frame[WINDOW];
    // int num_acks[WINDOW];
    int message_frame[WINDOW];
    char message_window[WINDOW][PACKET_SIZE];

    int canSend = 1;
    int sockfd;
    int rv;
    struct addrinfo hints, *servinfo;
    char portStr[10];


    totalRead = bytesToTransfer;
    
    FILE* fp;
    fp = fopen(filename, "rb");
    if (fp ==NULL) {
        fprintf(stderr, "File not opened\n");
        return;
    }

    memset(&hints, 0, sizeof hints);
    sprintf(portStr, "%d", hostUDPport);
    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    memset(&servinfo,0,sizeof servinfo);

    rv = getaddrinfo(hostname, portStr, &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        s = 1;
    }

    // loop through all the results
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("sender: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "sender: failed to bind socket\n");
        exit(1);
    }

    s = sockfd;

    memset(size_frame, 0, WINDOW * sizeof(int));
    memset(message_frame, 0, WINDOW * sizeof(int));
    memset(fileRead, 0, WINDOW * sizeof(int));
    memset(message_window, 0, WINDOW * (PACKET_SIZE));

    int framePointer = 0;
    char buf[BUFSIZE];
    expectedAck = 0;
    rtt = 20000; //time for 1 RTT

    while (1) {

        struct timeval RTT_TO;
        RTT_TO.tv_sec = 0;
        RTT_TO.tv_usec = 2 * rtt;


        struct timeval SENT, ACK_RECV;
        gettimeofday(&SENT, 0); //time of initial packet
        
        int i = framePointer;
        int readBytes;
        int num_acks = 0;

        for (int files = 0; files < canSend; files++) {
            char packet[PACKET_SIZE];
            memset(packet, 0, PACKET_SIZE);

            // If file is not read then read it and mark it
            if (fileRead[i] == 0) {
                fileRead[i] = 1;
                if(totalRead - bytes_read < PACKET_SIZE - HEADER_SIZE){
                    int remaining = totalRead - bytes_read;
                    if(remaining<=0) break; //all is read
                    readBytes = fread(message_window[i], 1, remaining, fp);
                    bytes_read += readBytes;
                }
                else{
                    readBytes = fread(message_window[i], 1, PACKET_SIZE - HEADER_SIZE, fp);
                    bytes_read += readBytes;
                }
                // Signals the end of the file
                if (readBytes < PACKET_SIZE - HEADER_SIZE) {
                    // lastBytes = readBytes;
                    lastIndex = i;
                }
                // Set the msg frame with the actual frame count
                message_frame[i] = frameCount++;
                size_frame[i] = readBytes;
            }
            else if(fileRead[i] == 1){
                readBytes = size_frame[i];   //time out pkts
            }
            else{
                if (i == lastIndex) {
                    break;
                }
                i++;
                i %= WINDOW;
                files--;
                continue;
            }
            //Account transmitted frame
            sprintf(packet, "frame%d;", message_frame[i]);
            
            int start = strlen(packet);
            memcpy(packet + start, message_window[i], readBytes);
            sendto(s,packet, start + readBytes, 0, p->ai_addr, p->ai_addrlen);
            
            if (i == lastIndex) {
                num_acks++;                // Trasnmit packet
                break;
            } else {
                num_acks++;
            }
            i++;
            i %= WINDOW;
        }
        int expectedAcks = num_acks;

        if (setsockopt(s, SOL_SOCKET,SO_RCVTIMEO,&RTT_TO,sizeof(RTT_TO)) < 0)
            fprintf(stderr, "Timeout socket err\n");

        if (expectedAcks == 0) {
            fprintf(stderr, "All expected ACKs received\n");
            break;
        }

        for (int ack = 0; ack < expectedAcks; ack++) {
            int recvbytes = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr *) & si_other, &slen);
            gettimeofday(&ACK_RECV, 0); // Time of receiving ack

            /*Time out*/
            if (recvbytes < 0) {
                counter++;
                if (canSend <= SLOW_DECLINE) {
                    canSend--;
                    canSend = (canSend > 1) ? canSend : 1;
                } else {
                    canSend /= 2;
                    canSend = (canSend > 1) ? canSend : 1;
                }
                if (counter >= 5) {
                    RTT_TO.tv_usec = ACK_RECV.tv_usec - SENT.tv_usec;
                    counter = 0;
                }

                break;
            }
            else { /** Ack received */
                buf[recvbytes] = '\0';
                int idx;
                char header[HEADER_SIZE];
                for (idx = 0; idx < recvbytes; idx++) {
                    header[idx] = buf[idx];
                    if (buf[idx] == ';') {
                        header[idx] = '\0';
                        idx++;
                        break;
                    }
                }

                int ackNum = 0;
                sscanf(header, "ack%d;", &ackNum);
                if (ackNum == expectedAck) {
                    expectedAck++;
                    fileRead[ackNum % WINDOW] = 0;
                    framePointer++;
                    framePointer %= WINDOW;
                    /*Congestion avoidance*/
                    if (canSend <= SLOW_GROWTH) { //step into CA phase
                        canSend++;
                        canSend = (canSend < WINDOW) ? canSend : WINDOW;
                    }
                    else { //slow start phase
                        slowGrowth += 1.0/canSend;
                        if (slowGrowth > 1.0) {
                            canSend++;
                            canSend = (canSend < WINDOW) ? canSend : WINDOW;
                            slowGrowth = 0;
                        }
                    }
                }
                else if (ackNum > expectedAck) {
                    fileRead[ackNum % WINDOW] = 2;

                    if (canSend <= SLOW_GROWTH) {
                        canSend++;
                        canSend = (canSend < WINDOW) ? canSend : WINDOW;
                    }
                    else{
                        slowGrowth += 1.0/canSend;
                        if(slowGrowth > 1.0){
                            canSend++;
                            canSend = (canSend < WINDOW) ? canSend : WINDOW;
                            slowGrowth = 0;
                        }
                    }
                }
            }

        }
    }

    int index;
    for (index = 0; index < 10; index++) {
        sendto(s,"EOT", sizeof("EOT"), 0, p->ai_addr, p->ai_addrlen);
    }

    fclose(fp);
    close(s);
    return;
}

int main(int argc, char** argv)
{
    unsigned short int udpPort;
    unsigned long long int numBytes;

    if(argc != 5)
    {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int)atoi(argv[2]);
    numBytes = atoll(argv[4]);

    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);
    return 0;
}

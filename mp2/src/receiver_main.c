/*
 * File:   receiver_main.c
 * Author:
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define BUFSIZE 2000
#define HEADER_SIZE 50
#define WINDOW 300

struct sockaddr_in si_me, si_other;
int s, slen;

void diep(char *s) {
    perror(s);
    exit(1);
}



void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {

    socklen_t slen = sizeof(si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");


    /* Now receive data and send acknowledgements */
    remove(destinationFile);
    FILE* fp=fopen(destinationFile,"wb");
    if (fp ==NULL) {
        fprintf(stderr, "FILE NOT OPENDED\n");
        return;
    }

    char buf[BUFSIZE];

    char charOOO[BUFSIZE*WINDOW]; //not in order messages
    int indexOOO[WINDOW];
    int beginOOO[WINDOW];
    int endOOO[WINDOW];

    int frameNumber = 0, sequenceNumber = 0, recvbytes = 0;
    int lastAckSent = -1;


    while (1) {
        recvbytes = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr*)&si_other, &slen);
        if (recvbytes < 0) {
            fprintf(stderr, "Connection closed\n");
            break;
        }

        buf[recvbytes] = '\0';

        if (!strncmp(buf, "EOT", 3)) {
            fprintf(stderr, "Done transmitting!\n");
            break;
        }
        else { //receiving packets
            char header[HEADER_SIZE];
            int k;
            char* curr;
            for (curr = buf, k = 0; k < BUFSIZE && *curr != ';'; curr++, k++) {
                header[k] = *curr;
            }

            curr++;     //pointing to first bit of message
            header[k] = '\0';
            int frameIndex;
            sscanf(header, "frame%d", &frameIndex);

            if (frameIndex == lastAckSent + 1) {
                printf("Current ACK received\n");
                lastAckSent++;
                fwrite(curr, 1, recvbytes - k - 1, fp);

                while(1){// remove excess acks from buffer
                    int i;
                    int storedlen = 0;
                    for(i = 0; i < frameNumber; ++i){
                        if(indexOOO[i] == lastAckSent + 1){
                            printf("Getting ACK from buffer\n");
                            lastAckSent++;
                            storedlen = endOOO[i] - beginOOO[i];
                            fwrite(&charOOO[beginOOO[i]], 1, storedlen, fp);
                            for(int k = endOOO[i]; k < sequenceNumber; ++k){
                                charOOO[k - storedlen] = charOOO[k];
                            }
                            for(int k = i + 1; k < frameNumber; ++k){
                                beginOOO[k - 1] = beginOOO[k];
                                endOOO[k - 1] = endOOO[k];
                                indexOOO[k - 1 ] = indexOOO[k];
                            }
                            break;
                        }
                    }
                    if(i!=frameNumber){
                        sequenceNumber -= storedlen;
                        frameNumber--;
                    }
                    else{
                        break;
                    }
                }

                char ack[HEADER_SIZE];
                sprintf(ack, "ack%d;", lastAckSent); //+1 stored has been used
                sendto(s, ack, strlen(ack), 0, (struct sockaddr *) & si_other, sizeof(si_other));

            }
            else if(frameIndex > lastAckSent){
                printf("Future ACK stored\n");
                char ack[HEADER_SIZE];
                sprintf(ack, "ack%d;", frameIndex);
                sendto(s, ack, strlen(ack), 0, (struct sockaddr *) & si_other, sizeof(si_other));

                //if it is out of order then store it
                beginOOO[frameNumber] = sequenceNumber;
                indexOOO[frameNumber] = frameIndex;
                int i;
                for(i = 0; i < recvbytes - k - 1; ++i){
                    charOOO[sequenceNumber] = *curr;
                    curr++;
                    sequenceNumber++;
                }
                endOOO[frameNumber] = sequenceNumber;
                frameNumber++;
            }
            else {
                printf("Getting previous ACK\n");
                char ack[HEADER_SIZE];
                sprintf(ack, "ack%d;", frameIndex); //frameIndex
                sendto(s, ack, strlen(ack), 0, (struct sockaddr *) & si_other, sizeof(si_other));
            }

            memset(buf, 0, BUFSIZE);//buf reads in a packet every time
        }
    }

    close(s);
    fclose(fp);
    printf("%s received.\n", destinationFile);
    return;
}

/*
 *
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

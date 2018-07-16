/*
    Simple udp server
    Silver Moon (m00n.silv3r@gmail.com)
*/
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
 
#define BUFLEN   512      // max length of buffer
#define PORTB    51717    // port on which to listen for incoming data (Machine B)
 
void die(char *s)
{
    perror(s);
    exit(1);
}
 
int main(void)
{
    // Create structures for Machine A and Machine B
    struct sockaddr_in si_A, si_B;
     
    int s, i, slen=sizeof(si_A), recv_len;
    char buf[BUFLEN];
     
    // Create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    // Zero out the structure for Machine B
    memset((char *) &si_B, 0, sizeof(si_B));

    si_B.sin_family = AF_INET;
    si_B.sin_port = htons(PORTB);
    si_B.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind UDP socket to port for Machine B
    if( bind(s , (struct sockaddr*)&si_B, sizeof(si_B) ) == -1)
    {
        die("bind");
    }
     
    // Try to receive some data from Machine A, this is a blocking call
    printf("Waiting for data...\r\n\n");
    fflush(stdout);

    if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_A, &slen)) == -1)
    {
        die("recvfrom()");
    }
         
    // Print details of Machine A and the data received
    printf("Received packet from %s: %d\n", inet_ntoa(si_A.sin_addr), ntohs(si_A.sin_port));
    printf("Data: %s\r\n" , buf);

    // Close socket
    close(s);
    printf("Socket closed.\r\n");
    return 0;
}

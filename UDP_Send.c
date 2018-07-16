/*
    Simple udp client
    Silver Moon (m00n.silv3r@gmail.com)
*/
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
 
#define SERVER "129.105.69.140"    // server IP address (Machine A)
#define PORTA   9090               // port on which to send data (Machine A)
#define BUFLEN  512                // max length of buffer
 

void die(char *s)
{
    perror(s);
    exit(1);
}
 

int main(void)
{
    // Create structure for Machine A
    struct sockaddr_in si_A;

    int s, i, slen=sizeof(si_A);
    char buf[BUFLEN];
    char message[BUFLEN];
 
    // Create a UDP socket
    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    // Zero out the structure for Machine A
    memset((char *) &si_A, 0, sizeof(si_A));

    si_A.sin_family = AF_INET;
    si_A.sin_port = htons(PORTA);

    // IP address of the receiver
    if (inet_aton(SERVER , &si_A.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    // Send the message to Machine A
    strcpy(message, "This is a test message.");
    if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_A, slen)==-1)
    {
        die("sendto()");
    }
    printf("Message sent to Machine A.\r\n");
 
    // Close socket
    close(s);
    printf("Socket closed.\r\n");
    return 0;
}

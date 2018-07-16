/*
    Simple udp client/server
    Silver Moon (m00n.silv3r@gmail.com)
*/

#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h> 
 
//#define SERVERA "129.105.69.140"    // server IP address (Machine A)
//#define PORTA    9090               // port on which to send data (Machine A)
//#define SERVERB "129.105.69.220"    // server IP address (Machine B)
#define PORTB    51717              // port on which to listen for incoming data (Machine B)
#define BUFLEN   512                // max length of buffer


// Create structures for Machine A and Machine B
struct sockaddr_in AddrMachineA, AddrMachineB;



void die(char *sock)
{
    perror(sock);
    exit(1);
}

int InitializeUDP(int Sock)
//void InitializeUDP(int *Sock)
{
    // Create a UDP socket
    if ((Sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
//    if ((*Sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
    printf("Socket created.\r\n");

/*
    // Zero out the structure for Machine A
    memset((char *) &AddrMachineA, 0, sizeof(AddrMachineA));

    AddrMachineA.sin_family = AF_INET;
    AddrMachineA.sin_port = htons(PORTA);

    // IP address of the receiver
    if (inet_aton(SERVERA, &AddrMachineA.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    printf("Machine A structure set up with IP address of the receiever and the specified port.\r\n");
*/

    // Zero out the structure for Machine B
    memset((char *) &AddrMachineB, 0, sizeof(AddrMachineB));


    AddrMachineB.sin_family = AF_INET;
    AddrMachineB.sin_port = htons(PORTB);
    AddrMachineB.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind UDP socket to port for Machine B
    if( bind(Sock, (struct sockaddr*)&AddrMachineB, sizeof(AddrMachineB) ) == -1 )
//    if( bind(*Sock, (struct sockaddr*)&AddrMachineB, sizeof(AddrMachineB) ) == -1 )
    {
        die("bind");
    }
    printf("UDP socket binded to any address and the specified port.\r\n");

    return Sock;
}


void ReceiveSocket(int sock, char buf[], int slen)
{
    // Try to receive some data from Machine A (blocking call)
    printf("Waiting for data...\r\n\n");
    fflush(stdout);

    int recv_len;
    if( (recv_len = recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &AddrMachineA, &slen)) == -1 )
    {
        die("recvfrom()");
    }
}




void SendSocket(int sock, char message[], int slen)
{
    // Send the message to Machine A
    if( sendto(sock, message, strlen(message) , 0 , (struct sockaddr *) &AddrMachineA, slen) == -1 )
    {
        die("sendto()");
    }
    printf("Message sent to Machine A.\r\n");
}



void CloseSocket(int sock)
{
    // Close socket
    close(sock);
    printf("Closed socket.\r\n");
}







int main(void)
{
    // Local variables used for UDP communication
    int Sock, slen=sizeof(AddrMachineA);
    char buf[BUFLEN];
    char message[BUFLEN];


    // Initialize socket
    Sock = InitializeUDP(Sock);
//    InitializeUDP(&Sock);


    // Receive message from Machine A
    int NUMIMAGES, FPS, PULSETIME, AMPL, FREQ, currentSample;
    float DELAYTIME;
    
    ReceiveSocket(Sock, buf, slen);

    printf("Received packet from %s: %d\n", inet_ntoa(AddrMachineA.sin_addr), ntohs(AddrMachineA.sin_port));
    printf("Unprocessed data: %s\r\n" , buf);

    sscanf(buf, "%d%*c %d%*c %d%*c %f%*c %d%*c %d%*c %d", &NUMIMAGES, &FPS, &PULSETIME, &DELAYTIME, &AMPL, &FREQ, &currentSample);
    printf("[NUMIMAGES, FPS, PULSETIME, DELAYTIME, AMPL, FREQ, currentSample]: %d, %d, %d, %f, %d, %d, %d\r\n", NUMIMAGES, FPS, PULSETIME, DELAYTIME, AMPL, FREQ, currentSample);

    sleep(3);

    // Send message to Machine A
    strcpy(message, "This is a test message.");
    SendSocket(Sock, message, slen);



    // Close socket
    CloseSocket(Sock);

    return 0;
}

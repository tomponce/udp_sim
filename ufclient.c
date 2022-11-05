#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER "129.120.151.96" // cse03
#define CLIENT "129.120.151.97" // cse04
#define BUFLEN 512  //Max length of buffer

void die(char *s)
{
    perror(s);
    exit(1);
}

struct udp_segment
{
    unsigned short int src_port;
    unsigned short int dest_port;
    unsigned short int length;
    unsigned short int checksum;
    char data[BUFLEN];
};

unsigned short checksum(struct udp_segment *);

int main(void)
{
    struct sockaddr_in si_client, si_server;
    struct udp_segment mysegment;
    int sockfd, portno, slen=sizeof(si_server);
    char filename[64];
    FILE *fp, *f_log;

    //get destination port number and input file
    memset((char *) &mysegment, 0, sizeof(mysegment));//first clear out segment
    printf("Port: ");
    scanf("%hu", &mysegment.dest_port); //assign to mysegment struct
    printf("Filename: ");
    scanf("%s", filename);
    getchar();

    //attempt to open file
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        die("file");
    }
    f_log = fopen("Client.log", "a");
    if (f_log == NULL)
    {
        die("file");
    }
    
    //create sockfd
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
 
    //set server socket
    memset((char *) &si_server, 0, sizeof(si_server));
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(mysegment.dest_port);
    if (inet_aton(SERVER , &si_server.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    //set client socket (for retrieving port no)
    memset((char *) &si_client, 0, sizeof(si_client));
    si_client.sin_family = AF_INET;
    si_client.sin_port = htons(0); //binding port 0 returns first available port
    if (inet_aton(CLIENT , &si_client.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    if(bind(sockfd, (struct sockaddr*)&si_client, sizeof(si_client) ) == -1)
    {
        die("bind");
    }

    //get client port number
    int clen = sizeof(si_client);
    getsockname(sockfd, (struct sockaddr *) &si_client, &clen);
    mysegment.src_port = htons(si_client.sin_port);

    //fill in rest of udp segment
    mysegment.length = 520;
    mysegment.checksum = 0;
    fgets(mysegment.data, BUFLEN, fp);
    mysegment.checksum = checksum(&mysegment); //compute checksum

    //print struct segments
    printf("Source Port: %d\n", mysegment.src_port);
    printf("Destination Port: %d\n", mysegment.dest_port);
    printf("Length: %d bytes\n", mysegment.length);
    printf("Checksum: %d (0x%X)\n", mysegment.checksum, mysegment.checksum);
    printf("Payload: %d bytes\n", sizeof(mysegment.data));

    //record everything in client log
    fprintf(f_log, "Source Port: %d\n", mysegment.src_port);
    fprintf(f_log, "Destination Port: %d\n", mysegment.dest_port);
    fprintf(f_log, "Length: %d bytes\n", mysegment.length);
    fprintf(f_log, "Checksum: %d (0x%X)\n", mysegment.checksum, mysegment.checksum);
    fprintf(f_log, "Payload: %d bytes\n", sizeof(mysegment.data));
    printf("Client log written\n");
    
    //send the message
    //take address of udp segment and cast it to type char *
    if (sendto(sockfd, (char *) &mysegment, sizeof(mysegment), 0, (struct sockaddr *) &si_server, slen) == -1)
    {
        die("sendto()");
    }

    fclose(fp);
    fclose(f_log);
    close(sockfd);
    return 0;
}

unsigned short checksum(struct udp_segment *segment)
{
    unsigned int sum = segment->src_port;
    //add source port and destination port
    sum += segment->dest_port;
    //check for carry
    while(sum & 0x10000) //0x10000 = 1 00000000 00000000
    {
        sum = (sum & 0xFFFF) + 1; //0xFFFF = 11111111 11111111
    }
    //add length to sum
    sum += segment->length;
    while(sum & 0x10000) //0x10000 = 1 00000000 00000000
    {
        sum = (sum & 0xFFFF) + 1; //0xFFFF = 11111111 11111111
    }
    unsigned short temp;
    for(int i = 0; i < 256 && segment->data[2 * i]; i++)
    {
        temp = ((unsigned short *)(segment->data))[i]; //treat paylod as unsigned short array
        sum += temp;
        while(sum & 0x10000) 
        {
            sum = (sum & 0xFFFF) + 1;
        }
    }
    //get complement
    unsigned short complement = (unsigned short) sum ^ 0xFFFF; //0xFFFF = 11111111 11111111
    //sum = sum ^ 0xFFFF; //0xFFFF = 11111111 11111111
    return complement;
}
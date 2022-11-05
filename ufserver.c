#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER "129.120.151.96" // cse03
#define CLIENT "129.120.151.97" // cse04
//#define PORT 6754 //source port number is hardcoded
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

unsigned short checksum(struct udp_segment *segment);

int main(void)
{
    struct sockaddr_in si_server, si_client;
    struct udp_segment mysegment;
    int sockfd, portno, clen = sizeof(si_client) , recv_len;
    FILE *f_write, *f_log;
    
    //get destination port number
    printf("Port: ");
    scanf("%d", &portno);

    //create a UDP socket descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_server, 0, sizeof(si_server));
    // fill in sock struct segments
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(portno);
    si_server.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if(bind(sockfd, (struct sockaddr*)&si_server, sizeof(si_server) ) == -1)
    {
        die("bind");
    }

    //open file for wrting
    f_write = fopen("output.txt", "w");
    if (f_write == NULL)
    {
        die("file");
    }
    //open file for logging output
    f_log = fopen("Server.log", "a");
    if (f_log == NULL)
    {
        die("file");
    }
    
    fflush(stdout);
    
    memset((char *) &mysegment, 0, sizeof(mysegment));    
    //try to receive some data, this is a blocking call
    if ((recv_len = recvfrom(sockfd, (char *) &mysegment, sizeof(mysegment), 0, (struct sockaddr *) &si_client, &clen)) == -1)
    {
        die("recvfrom()");
    }

    //print segments received from client
    printf("Source Port: %d\n", mysegment.src_port);
    printf("Destination Port: %d\n", mysegment.dest_port);
    printf("Length: %d bytes\n", mysegment.length);
    printf("Checksum: %d (0x%X)\n", mysegment.checksum, mysegment.checksum);
    printf("Payload: %d bytes\n", sizeof(mysegment.data));

    //compute if checksum matches
    unsigned short check_s = checksum(&mysegment);
    printf("Computed Checksum: %d (0x%X)\n", check_s, check_s);
    if(mysegment.checksum != check_s)
    {
        printf("Checksum mismatch!\n");
        printf("Output.txt not written\n");
        printf("Server log not written\n");
        fclose(f_write);
        fclose(f_log);
        close(sockfd);
        return -1;
    }
    printf("Checksum matched!\n");

    //write data payload to output.txt if success
    fprintf(f_write, "%s", mysegment.data);
    printf("Output.txt written\n");

    //record everything in server log
    fprintf(f_log, "Source Port: %d\n", mysegment.src_port);
    fprintf(f_log, "Destination Port: %d\n", mysegment.dest_port);
    fprintf(f_log, "Length: %d bytes\n", mysegment.length);
    fprintf(f_log, "Checksum: %d (0x%X)\n", mysegment.checksum, mysegment.checksum);
    fprintf(f_log, "Payload: %d bytes\n", sizeof(mysegment.data));
    printf("Server log written\n");
 
    fclose(f_write);
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
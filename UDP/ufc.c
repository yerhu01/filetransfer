#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define SEND_BUFF_SIZE 100
#define RECV_BUFF_SIZE 1024
#define SERV_PORT_NO 8080
#define SERV_IP_ADDR  "127.0.0.1"
//#define SERV_IP_ADDR "10.10.1.100"
int main(int argc, char* argv[]){
	
	if (argc < 2){
		fprintf(stderr, "You must provide a filename\n");
		exit(1);
    }
	
	// Initialize 
    int sockfd = 0;  //socket file descriptor
    int  bytesReceived = 0;
    int bytesSent = 0;
    //ssize_t recsize; not used
    char sendBuff[SEND_BUFF_SIZE];  //buffer for file name
    char recvBuff[RECV_BUFF_SIZE];  //buffer for filedata
    //memset(recvBuff, '0', sizeof(recvBuff)); //resets recvBuff to 0
    struct sockaddr_in sa; //for IPv4 address (compared to sockaddr), can be cast into sockaddr
    socklen_t length;  //size of sa
	
    /* Create a socket first */
    if((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP))< 0){
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    /* Zero out socket address */
    memset(&sa, 0, sizeof sa);
    /* Initialize sockaddr_in data structure */
    sa.sin_family = AF_INET;
    sa.sin_port = htons(SERV_PORT_NO); // port. htons converts int from host byte order to network byte order
    sa.sin_addr.s_addr = inet_addr(SERV_IP_ADDR); //inet_addr covert the string IP to the data structure sin_addr
    length = sizeof sa;

    strcpy(sendBuff, argv[1]);  //argv[1] = filename  

    /*Send the filename*/
    bytesSent = sendto(sockfd, sendBuff, strlen(sendBuff), 0,(struct sockaddr*)&sa, sizeof sa);
    if (bytesSent < 0){
        printf("Sendto: Error sending the file name: %s\n", strerror(errno));
        return 1;
    }    

    printf("Filename sent!\n");    

    /* Create file where data will be stored */
    FILE *fp;
    fp = fopen(argv[1], "w"); // will write to filename in argv[1]
    if(NULL == fp){
        printf("Error opening the file");
        return 1;
    }

    printf("Begin receiving the file...\n");
	for(;;){
		/* Receive data in chunks of 1024  bytes */
		bytesReceived =recvfrom(sockfd,recvBuff,sizeof recvBuff ,0,  (struct sockaddr *)&sa, &length);
		if (bytesReceived<0){
			printf("Recvfrom: Error in receiving the file\n");
			exit(1);
		}else{   
			printf("Number of bytes received: %d\n", bytesReceived); //always 1024 (see ufs.c)

		}
		
		if(recvBuff[RECV_BUFF_SIZE-1]=='1'){
			//end of file: writes recvBuff up to '/0' into fp file
			if(fwrite(recvBuff,1,strlen(recvBuff),fp)<0){
				printf("Error writing to file\n");
				exit(1);
			}
			printf("End of file \n");
			break; //leave for loop, client ends
		}else if(recvBuff[bytesReceived-1]=='2'){
			//file name not found
			fprintf(stderr,"Not Found\n");
			exit(1);
		}else{
			//writes 1023 bytes of recvBuff to fp (file)
			if(fwrite(recvBuff,1,bytesReceived-1,fp)<0){
				printf("Error writing to file\n");
				exit(1);
			}
		}
		memset(recvBuff, '0', sizeof(recvBuff)); //resets recvBuff to 0's
    }
	fclose(fp);
	close(sockfd);
	return 0;
}

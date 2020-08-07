#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include <dirent.h>

#define RECV_BUFF_SIZE 100
#define SEND_BUFF_SIZE 1024
#define PORT_NO 8080
int main(int argc, char* argv[]){

	if (argc < 2){
		fprintf(stderr, "You must provide a directory name\n");
		exit(1);
    }
	// Open the directory we want
	DIR *dp = opendir(argv[1]);
	if(dp== NULL){
		fprintf(stderr,"Cannot open directory: %s\n", argv[1]);
		exit(1);
	}
	/*go to the directory*/
	chdir(argv[1]); 
	
	/* Initialize */
	int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); //socket family, type, protocol
	struct sockaddr_in sa; //socket address
	char recvBuff[RECV_BUFF_SIZE]; //buffer to receive message
	ssize_t recsize;  //# of bytes received
	socklen_t fromlen; //for recvfrom(), size of sa
	char sendBuff[SEND_BUFF_SIZE];  //buffer to send file data

	memset(sendBuff, 0, sizeof(sendBuff));

	/* Zero out socket address */
	memset(&sa, 0, sizeof sa);
	/* Initialize sockaddr_in data structure */
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);  //listen for any client.
	sa.sin_port = htons(PORT_NO);          //port. htons converts int from host byte order to network byte order
	fromlen = sizeof(sa);

	/* server binds to socket */
	if (-1 == bind(sock, (struct sockaddr *)&sa, sizeof sa)){
		perror("error bind failed");
		close(sock);
		return 1;
	}
	
	//continue listening for client
	for (;;){
		recsize = recvfrom(sock, (void*)recvBuff, sizeof recvBuff, 0, (struct sockaddr*)&sa, &fromlen);
		if (recsize < 0){
			fprintf(stderr, "%s\n", strerror(errno));
			return 1;
        }
		printf("Requested filename: %.*s\n", (int)recsize, recvBuff);
		
		/* Open the file that we wish to transfer */
		FILE *fp = fopen(recvBuff,"r");
		if(fp==NULL){
            fprintf(stderr, "Not Found\n");
			sendBuff[SEND_BUFF_SIZE-1]='2'; //indicates "Not Found" message to client
			sendto(sock,sendBuff, SEND_BUFF_SIZE, 0,(struct sockaddr*)&sa, sizeof sa); //sends Not Found message
            return 1;
        }

		//sending the file in chunks
		while(!feof(fp)){
            /* Read file in chunks of 1023 bytes, last byte reserved for end of file flag */           
            int nread = fread(sendBuff,1,SEND_BUFF_SIZE-1,fp);
			if(feof(fp)){
				//end of file
				sendBuff[nread] = '\0'; //strlen
				sendBuff[SEND_BUFF_SIZE-1]='1'; // end of file flag on
				printf("End of file\n");
			}else{
				//not end of file
				sendBuff[SEND_BUFF_SIZE-1] = '0'; //end of file flag off
			}
            printf("Bytes read %d \n", nread);
			
            /* Send data. */
			/*if the buffer is filled exactly with 1023 bytes on the LAST chunk, another empty chunk with 1 flag will still be sent*/
            printf("Sending the file ...\n");
            int n = sendto(sock,sendBuff, SEND_BUFF_SIZE, 0,(struct sockaddr*)&sa, sizeof sa); //always sending BUFF_SIZE
            if(n<0){
                perror("Problem sendto\n");
                exit(1);
            }
			
			if (ferror(fp)){
				printf("Error reading the file at server program\n");
				break;
			}
			usleep(1000); //delay so doesn't get out of order
        }
		fclose(fp);
	}
	close(sock);
	return 0;
}

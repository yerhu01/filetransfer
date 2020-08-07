#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include<string.h>

#include <unistd.h> //for close(), write(), send() otherwise implicit declaration
#include <arpa/inet.h> //for inet_pton, otherwise implicit declaration
#include <time.h>
#include <stdlib.h>

#define SEND_BUFF_SIZE 1024
#define RECV_BUFF_SIZE 1024
int main(int argc,char* argv[]){
    
    if (argc < 4){
		fprintf(stderr, "You must provide IP address, Port number, and name of a zipped file\n");
		exit(1);
    }
    
    /* Initialize */
	int sockfd, n;
    char sendBuff[SEND_BUFF_SIZE];
    char recvBuff[RECV_BUFF_SIZE];
    
    struct sockaddr_in servaddr;
 
    sockfd=socket(AF_INET,SOCK_STREAM,0);  //SOCKET(), open a connection oriented TCP socket 
	
	/* Zero out socket address */
	bzero(&servaddr,sizeof servaddr);
	/* Initialize sockaddr_in data structure */
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(atoi(argv[2])); //port = 8080
 
    inet_pton(AF_INET,argv[1],&(servaddr.sin_addr)); //argv[1] = 127.0.0.1 for home use, 10.10.1.100 for flexinet use
 
    /*configure socket to reuse ports*/
    //int optval = 1;
    //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr)); //CONNECT() to a server socket, CLIENT SIDE only
 
    	//connection established
	clock_t start = clock(), diff; //start timer

	/* Send the zipped file name */
	strncpy(sendBuff, argv[3], strlen(argv[3]));
	sendBuff[strlen(argv[3])] = '\0';
	n = send(sockfd,sendBuff,strlen(sendBuff)+1,0); //send null too
	if(n<0){
		perror("Problem file size send()\n");
		exit(1);
	}
	printf("Sent zipped file name: %s\n",sendBuff);
	printf("Bytes sent %d \n", n);
	
	/* Open the file that we wish to transfer */
	FILE *fp = fopen(argv[3],"r");
	if(fp==NULL){
        fprintf(stderr, "File Not Found\n");
    }
	
	/* Send the zipped file size */
	//seek end of file and get position
	fseek(fp, 0L, SEEK_END);
	int size = ftell(fp);
	//reset to beginning
	fseek(fp, 0L, SEEK_SET);
	printf("Zipped File size: %d\n",size);
	n = send(sockfd,&size,sizeof(size),0);
	if(n<0){
		perror("Problem file size send()\n");
		exit(1);
	}
	printf("Zipped File Size Bytes sent %d \n", n);
	
    	/* Send the file data */
	printf("Begin sending the zipped file...\n");
	while(!feof(fp)){
        /* Read file in chunks of up to 1024 bytes */
        int bytesRead = fread(sendBuff,1,SEND_BUFF_SIZE,fp);
		if (bytesRead<0){
			printf("Error in reading the zip file\n");
			exit(1);
		}
        //printf("Bytes read %d \n", bytesRead);
		
		//printf("%s\n", sendBuff);
		
        /* Send data. */
		//printf("Sending the zipped file ...\n");
		n = send(sockfd, sendBuff, bytesRead,0); //no flags
		if(n<0){
				perror("Problem zipped file send()\n");
				exit(1);
		}
		//printf("Bytes sent %d \n", n);
		memset(sendBuff, '0', sizeof(sendBuff));  //reset sendBuff
	}	
	
	printf("Done sending zipped file\n");
	fclose(fp);

	/* Receive unzipped file name */	
	n = recv(sockfd,recvBuff,RECV_BUFF_SIZE,0);
	if(n<0){
		perror("Problem file size send()\n");
		exit(1);
	}
	char unzipfilename[strlen(recvBuff)+1];
	unzipfilename[strlen(recvBuff)] = '\0'; //set null char
	strncpy(unzipfilename, recvBuff, strlen(recvBuff));
	printf("Unzipped File name received: %s\n", unzipfilename);
	printf("Bytes received %d \n", n);

	/* Create file where data will be stored */
    	FILE *fp2;
    	fp2 = fopen(unzipfilename, "w"); //----------------change this to zipfilename "temp.txt"
    	if(NULL == fp2){
    	    printf("Error opening the file temp.zip");
    	    return 1;
    	}

	
	/* Receive unzipped file size */
	//int size; //stores size of receiving file
	n = recv(sockfd,&size,sizeof(size),0);
	if(n<0){
		perror("Problem file size recv()\n");
		exit(1);
	}
	printf("Unzipped File size: %d\n",size);
	printf("Unzipped File Size Bytes received %d \n", n);

	/* Receive unzipped file data */
	printf("Begin receiving the unzipped file...\n");	
    for(;;){
		if(size > RECV_BUFF_SIZE){
			//not end of file
			int bytesReceived = recv(sockfd, recvBuff, RECV_BUFF_SIZE, 0);
			if (bytesReceived<0){
				printf("recv(): Error in receiving the file\n");
				exit(1);
			}
			//printf("Bytes received %d \n", bytesReceived);
			size = size - bytesReceived;
			
			//printf("%s\n", recvBuff);
			
			int nwrite = fwrite(recvBuff, 1, bytesReceived,fp);
			if(nwrite < 0){
				printf("Error writing to temp.zip\n");
				exit(1);
			}
			//printf("Bytes written %d \n", nwrite);
		}else{
			//end of file
			int bytesReceived = recv(sockfd, recvBuff, size, 0);
			if (bytesReceived<0){
				printf("recv(): Error in receiving the file\n");
				exit(1);
			}
			//printf("Bytes received %d \n", bytesReceived);
			
			//printf("%s\n", recvBuff);
			
			int nwrite = fwrite(recvBuff, 1, bytesReceived,fp);
			if(nwrite < 0){
				printf("Error writing to temp.zip\n");
				exit(1);
			}
			//printf("Bytes written %d \n", nwrite);
			printf("End of file. Finished receiving unzipped file.\n");
			memset(recvBuff, '0', sizeof(recvBuff)); //resets recvBuff to 0's
			break;
		}
		memset(recvBuff, '0', sizeof(recvBuff)); //resets recvBuff to 0's
	}

	fclose(fp2);

	diff = clock() - start;
	int msec = diff * 1000 / CLOCKS_PER_SEC;
	printf("Time taken: %d milliseconds\n", msec%1000);

	close(sockfd);
	return 0;
}

/*Required Headers*/
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h> //for close(), write(), send() otherwise implicit declaration
#include <arpa/inet.h> //for inet_addr
#include <stdlib.h>

#define SEND_BUFF_SIZE 1024
#define RECV_BUFF_SIZE 1024
int main(int argc, char* argv[]){
    
    if (argc < 3){
	    fprintf(stderr, "You must provide IP address and Port number\n");
        exit(1);
    }
    
    /* Initialize */
    char sendBuff[SEND_BUFF_SIZE];
    char recvBuff[RECV_BUFF_SIZE];
    int listen_fd, comm_fd, n;
 
    struct sockaddr_in servaddr;
 
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);  //SOCKET(), open a connection oriented TCP socket
 
 
    /* Zero out socket address */
    bzero( &servaddr, sizeof(servaddr)); //could use memset instead
    /* Initialize sockaddr_in data structure */
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]); //listen for given client IP address //htons(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[2])); //socket port number. htons converts int from host byte order to network byte order //htons(8080);
 
 
    bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)); //BIND() TCP socket to a local address
 
    /*configure socket to reuse ports*/
    //int optval = 1;
    //setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    listen(listen_fd, 10); //first socket LISTEN() for TCP connection, SERVER side only. backlog (listen queue size) = 10
 
    comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL); //ACCEPT(), outputs new socket,using this able to receive the info from client side, SERVER side only
 
    //connection established
    /* Receive zipped file name */	
    n = recv(comm_fd,recvBuff,RECV_BUFF_SIZE,0);
    if(n<0){
        perror("Problem file size send()\n");
        exit(1);
    }
    //printf("file %s\n", recvBuff);
    char zipfilename[strlen(recvBuff)+1];
    zipfilename[strlen(recvBuff)] = '\0'; //set null char
    char unzipfilename[strlen(recvBuff)+1];
    unzipfilename[strlen(recvBuff)] = '\0'; //set null char
    strncpy(zipfilename, recvBuff, strlen(recvBuff));
    strncpy(unzipfilename, recvBuff, strlen(recvBuff));
    char* ptr = unzipfilename;
	ptr = ptr + strlen(unzipfilename)-4;
	strncpy(ptr, ".txt", strlen(".txt"));
	printf("Zipped File name received: %s\n", zipfilename);
	//printf("Unzipped File name received: %s\n", unzipfilename);
	printf("Zipped File name bytes received %d \n", n);
	
	/* Create file where data will be stored */
    FILE *fp;
    fp = fopen(zipfilename, "w"); //----------------change this to zipfilename "temp.txt"
    if(NULL == fp){
        printf("Error opening the file temp.zip");
        return 1;
    }
	
	/* Receive file size */
	int size; //stores size of receiving file
	n = recv(comm_fd,&size,sizeof(size),0);
	if(n<0){
		perror("Problem file size recv()\n");
		exit(1);
	}
	printf("File size: %d\n",size);
	printf("File Size Bytes received %d \n", n);
	
	/* Receive file data */
	printf("Begin receiving the zipped file...\n");	
    for(;;){
		if(size > RECV_BUFF_SIZE){
			//not end of file
			int bytesReceived = recv(comm_fd, recvBuff, RECV_BUFF_SIZE, 0);
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
			int bytesReceived = recv(comm_fd, recvBuff, size, 0);
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
			printf("End of file. Finished receiving zipped file.\n");
			memset(recvBuff, '0', sizeof(recvBuff)); //resets recvBuff to 0's
			break;
		}
		memset(recvBuff, '0', sizeof(recvBuff)); //resets recvBuff to 0's
	}
	
	fclose(fp);
	
	//unzip received zip file
	char command[50];
	strcpy(command, "unzip ");
	strncat(command, zipfilename, strlen(zipfilename));
	system(command);
	
	//Open unzipped file that we want to transfer back
	FILE *fp2 = fopen(unzipfilename,"r");
	if(fp2==NULL){
		fprintf(stderr,"Unzipped File Not Found\n");
		return 1;
	}

	//Send unzipped file name
	n = send(comm_fd,unzipfilename,strlen(unzipfilename)+1,0); //send null too
	if(n<0){
		perror("Problem file size send()\n");
		exit(1);
	}
	printf("Sent unzip file name: %s\n",unzipfilename);
	printf("Bytes sent %d \n", n);
	
	//Send unzipped file size
	//seek end of file and get position
	fseek(fp2, 0L, SEEK_END);
	size = ftell(fp2);
	//reset to beginning
	fseek(fp2, 0L, SEEK_SET);
	printf("Unzip File size: %d\n",size);
	n = send(comm_fd,&size,sizeof(size),0);
	if(n<0){
		perror("Problem file size send()\n");
		exit(1);
	}
	printf("Unzip File Size Bytes sent %d \n", n);

	/* Send the unzipped file data */
	printf("Begin sending the unzipped file...\n");
	while(!feof(fp2)){
        /* Read file in chunks of up to 1024 bytes */
        int bytesRead = fread(sendBuff,1,SEND_BUFF_SIZE,fp2);
		if (bytesRead<0){
			printf("Error in reading the zip file\n");
			exit(1);
		}
        //printf("Bytes read %d \n", bytesRead);
		
		//printf("%s\n", sendBuff);
		
        /* Send data. */
		//printf("Sending the unzipped file ...\n");
		n = send(comm_fd, sendBuff, bytesRead,0); //no flags
		if(n<0){
				perror("Problem unzipped file send()\n");
				exit(1);
		}
		//printf("Bytes sent %d \n", n);
		memset(sendBuff, '0', sizeof(sendBuff));  //reset sendBuff
	}	
	printf("Done sending unzipped file\n");
	fclose(fp2);

	close(comm_fd);
	close(listen_fd);
	return 0;
}

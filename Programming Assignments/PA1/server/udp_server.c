/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 * Modified by: Kiran Jojare
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MAX_PAC_SIZE	100
#define BUFSIZE 		1024
#define DATABUFSIZE 	(MAX_PAC_SIZE+5)
#define MAX_CMD			5
#define ACK				0x01
#define NACK			0xFF

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

//commands to be sent
enum cmd_names{
CMD_PUT,
CMD_GET,
CMD_DEL,
CMD_LS,
CMD_EXIT,
NO_CMD
};

bool closegrace = false;
char cmd_table[MAX_CMD][12] = { "put", "get", "delete", "ls", "exit"};
char cmd_size[MAX_CMD] = { 3, 3, 6, 2, 4};
char cmd_arg[MAX_CMD] = { 2, 2, 2, 1, 1};
enum cmd_names cmd_index=0;
char filename[200] = {0};//"/home/Kiran/NETSYS/PA1_udp_example/NetSys-Assignment1/server/";
int fd=0;
int sockfd, n; /* socket */
int clientlen; /* byte size of client's address */
struct sockaddr_in clientaddr; /* client addr */

/*
 * brief: Process a received command string, and send response.
 * input: The null-terminated string as typed by the user
 * reference: from PES assignment 6 - Howdy Pierce, modified as per requirement
 */
void process_command(char *input)
{
	char *p = input;
	char *end;

	// find end of string
	for (end = input; *end != '\0'; end++)
		;

		// Lexical analysis: Tokenize input in place
	bool in_token = false;
	char *argv[10];
	int argc = 0;
	memset(argv, 0, sizeof(argv));
	for (p = input; p < end; p++) 
	{
		if (in_token && isspace(*p)) 
		{
			*p = '\0';
			in_token = false;
		} 
		else if (!in_token && !isspace(*p)) 
		{
			argv[argc++] = p;
			in_token = true;
			if (argc == sizeof(argv)/sizeof(char*) - 1)
				// too many arguments! drop remainder
				break;
		}
	}

	argv[argc] = NULL;
	if (argc == 0)   // no command
		return;
		
	// get command to perform commands respective operation
	for (cmd_index = 0; cmd_index<MAX_CMD; cmd_index++)
	{
		if((strncasecmp(argv[0], &cmd_table[cmd_index][0], cmd_size[cmd_index])) == 0)
		{
			if (argc != cmd_arg[cmd_index])
				cmd_index = MAX_CMD;
			break;
		}
	}
		
	//for get, put, delete cmds - to get filename
	bzero(filename, 200);
	if (cmd_index == 2)
	{
		strncpy(filename, "rm ", 3);  
		strncat(filename, argv[1], strlen(argv[1]));  
	}
	else if(cmd_index < 2)
	{
		strncpy(&filename[0], argv[1], strlen(argv[1]));
	}
}

/*
 * brief: receives file data packet by packet
 * fixed size of 100bytes is used per packet 
 */
void reliable_put()
{	
    char *str = NULL;
    char databuf[DATABUFSIZE] = {0};
    int bytes = 0;
	char packet = 0;
	uint32_t parity = 0;
		
	printf("Receiving file %s\n", &filename[0]);
		
	while(1)
	{
		bzero(databuf, DATABUFSIZE);
		str = &databuf[0];
		
		//receive packet from client
		n = recvfrom(sockfd, databuf, DATABUFSIZE, 0,
		 	(struct sockaddr *) &clientaddr, &clientlen);
		if (n < 0)
		{
		  error("ERROR in recvfrom");
		}
		  
		// if file is completely sent, break loop 
		if(strncmp(databuf, "done", 4) == 0)
		{
			break;
		}

		// first byte packet number, second byte number of data bytes in packet
		packet = databuf[0];
		bytes = databuf[1];
		
		// increment pointer to point to actual data
		*str++;
		*str++;
		
		// if less bytes received then send NACK and go back to receive same packet again
		if ((n <= 1) || (bytes != n-2))
		{
			databuf[0] = packet;
			databuf[1] = NACK;
			n = sendto(sockfd, databuf, strlen(databuf), 0, 
					   (struct sockaddr *) &clientaddr, clientlen);
			if (n < 0) 
			  error("ERROR in sendto");
			printf("Packet %c resend\n", packet);
			continue;
		}

		// write (max 100) received bytes to file 
		n = write(fd, str, bytes);
		if(n == -1)
		{
        	error("ERROR writing file");
        	break;
		} 
		
		// send ACK on successful reception of packet
		bzero(databuf, DATABUFSIZE);
		// first byte packet number, second ACK
		databuf[0] = packet;
		databuf[1] = ACK;
		n = sendto(sockfd, databuf, strlen(databuf), 0, 
				   (struct sockaddr *) &clientaddr, clientlen);
		if (n < 0) 
		  error("ERROR in sendto");
		
		//printf("Packet %c received successfully\n", packet);
	}
	//close file before stopping 'put' operation
	close(fd);
	printf("Received file %s successfully\n", &filename[0]);
}

/*
 * brief: sends file data packet by packet
 * fixed size of 100bytes is used 
 */
void reliable_get()
{	
    char *str = NULL;
    char databuf[DATABUFSIZE] = {0};
    int ret = 0;
	char packet = '0';
	int prv_bytes = 0;
    
	printf("Sending file %s\n", &filename[0]);
			
	while(1)
	{
		bzero(databuf, DATABUFSIZE);
		str = &databuf[0];
		// first byte packet number
		*str++ = packet;
		*str++; 
		// Read max 100 bytes from file
		ret = read(fd, str, MAX_PAC_SIZE);
		if(ret == -1)
		{
        	error("ERROR reading file");
        	break;
		} 
		// End of file stop packets, can be made more reliable by checking filesize 
		if(ret == 0)
		{
			printf("done sending\n");
			n = sendto(sockfd, "done", 5, 0, (struct sockaddr *) &clientaddr, clientlen);
			if (n < 0) 
			  error("ERROR in sendto");
			break;
		}

		// second byte - number of bytes in packet for verification at other end for missing data
		databuf[1] = ret;
		
		//send 1st-Packet, 2nd-Bytes, 3rd to 102nd - data	
		n = sendto(sockfd, databuf, ret+2, 0, (struct sockaddr *) &clientaddr, clientlen);
		if (n < 0) 
		  error("ERROR in sendto");
		
		//clear buf before recieving ACK
		bzero(databuf, DATABUFSIZE);
		/* print the server's reply */
		n = recvfrom(sockfd, databuf, 6, 0,
		 	(struct sockaddr *) &clientaddr, &clientlen);
		if (n < 0) 
		  error("ERROR in recvfrom");	

		// In case of NACK or any other response resend package, need to add timeout for more reliable transfer  		  
		if(databuf[1] == NACK)
		{
			printf("Packet %c resend\n", databuf[0]);
			//lseek to previous data bytes to resend packet
			printf("lseek = %ld\n", lseek(fd, prv_bytes, SEEK_SET));
		}
		else if (packet == databuf[0])
		{
			//printf("Packet %c recieved successfully\n", databuf[0]);
			prv_bytes += ret;	//track of data bytes sent
			//reuse packet numbers from '0' to '9'
			packet++;
			if(packet > '9')
				packet = '0';
		}
		else
		{
			printf("Packet %c resend\n", databuf[0]);
			//lseek to previous data bytes to resend packet
			printf("lseek = %ld\n", lseek(fd, prv_bytes, SEEK_SET));
		}
	}
	//close file before stopping put operation
	close(fd);
	printf("Sent file %s successfully\n", &filename[0]);
}

/*
 * brief: main socket communication
 * client code to execute put, get, delete, ls. exit operations
 */
int main(int argc, char **argv) 
{
	//int sockfd; /* socket */
	int portno; /* port to listen on */
	//int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	//struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	char buf[BUFSIZE]; /* message buf */
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */
		FILE *fp = NULL;

	/* 
	* check command line arguments 
	*/
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	portno = atoi(argv[1]);

	/* 
	* socket: create the parent socket 
	*/
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");

	/* setsockopt: Handy debugging trick that lets 
	* us rerun the server immediately after we kill it; 
	* otherwise we have to wait about 20 secs. 
	* Eliminates "ERROR on binding: Address already in use" error. 
	*/
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, 
			(const void *)&optval , sizeof(int));
	/*
	* build the server's Internet address
	*/
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)portno);

	/* 
	* bind: associate the parent socket with a port 
	*/
	if (bind(sockfd, (struct sockaddr *) &serveraddr, 
		sizeof(serveraddr)) < 0) 
		error("ERROR on binding");

	/* 
	* main loop: wait for a datagram, then echo it
	*/
	clientlen = sizeof(clientaddr);
	while (1) 
	{
		/*
		* recvfrom: receive a UDP datagram from a client
		*/
		bzero(buf, BUFSIZE);
		n = recvfrom(sockfd, buf, BUFSIZE, 0,
			(struct sockaddr *) &clientaddr, &clientlen);
		if (n < 0)
		error("ERROR in recvfrom");

		/* 
		* gethostbyaddr: determine who sent the datagram
		*/
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
				sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL)
		error("ERROR on gethostbyaddr");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
		error("ERROR on inet_ntoa\n");
		printf("server received datagram from %s (%s)\n", 
		hostp->h_name, hostaddrp);
		printf("server received %ld/%d bytes: %s\n", strlen(buf), n, buf);
		
		// process command typed by user  
		process_command(buf);
		
		// operation as per command
		switch (cmd_index)
		{
			case CMD_PUT:
				// open or create given file
				fd = open(&filename[0], O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
				if (fd == -1)
					{
						error("ERROR opening file");
						break;
					}
					
				// send confirmation to client about file
				n = sendto(sockfd, "File created", 13, 0, 
					(struct sockaddr *) &clientaddr, clientlen);
				if (n < 0) 
				error("ERROR in sendto");
				
				reliable_put();    		
			break;
			
			case CMD_GET:
				// open file to be sent as per given name
				fd = open(&filename[0], O_RDWR, S_IRWXU);
				if (fd == -1)
				{	
					// if not present send not found to client
					n = sendto(sockfd, "File not found", 15, 0, 
					(struct sockaddr *) &clientaddr, clientlen);
					if (n < 0) 
						error("ERROR in sendto");
					printf("ERROR opening file");
				}
				else
				{		
					// if prsent send found to client
					n = sendto(sockfd, "File found", 11, 0, 
					(struct sockaddr *) &clientaddr, clientlen);
					if (n < 0) 
						error("ERROR in sendto");
					
					reliable_get(); 
				}
			break;
			
			case CMD_DEL:
				// call rm <filename> using system call
				printf("%s\n", filename);
				system(filename);

				// send details to client
				n = sendto(sockfd, filename, strlen(filename), 0, 
					(struct sockaddr *) &clientaddr, clientlen);
				if (n < 0) 
				error("ERROR in sendto");
			break;
			
			case CMD_LS:
				// get output of 'ls' command in a file
				fp = popen("ls *", "r");
				if (fp == NULL)
				error("ERROR in popen");
							
				// send that file data to client
				fread(buf, 1, BUFSIZE, fp);
				printf("%s\n", buf);
				n = sendto(sockfd, buf, strlen(buf), 0, 
					(struct sockaddr *) &clientaddr, clientlen);
				if (n < 0) 
				error("ERROR in sendto");

				// close opened file
				fclose(fp);
			break;
			
			case CMD_EXIT:
				// send exiting status
				printf("Server exiting\n");
				n = sendto(sockfd, "Server exiting", 15, 0, 
					(struct sockaddr *) &clientaddr, clientlen);
				if (n < 0) 
				error("ERROR in sendto");
				closegrace = true;
			break;
			
			default:
				/* 
				* sendto: echo the input back to the client 
				*/
				n = sendto(sockfd, buf, strlen(buf), 0, 
					(struct sockaddr *) &clientaddr, clientlen);
				if (n < 0) 
				error("ERROR in sendto");
			break;
		}
		
		// on 'exit' command break loop and close connection
		if (closegrace)
		{
			close(sockfd);
			break;
		}
	}
	return 0;
}






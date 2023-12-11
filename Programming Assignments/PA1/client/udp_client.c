/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 * reference: https://pubs.opengroup.org/onlinepubs/009696799/functions/popen.html
 * Modified by: KiranJojare
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdbool.h>
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
    exit(0);
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

char filename[200] = {0};
int fd = 0;
int sockfd, n;
int serverlen;
struct sockaddr_in serveraddr;

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
				cmd_index = NO_CMD;
			break;
		}
	}

	//for get, put, delete cmds - to get filename
	if (cmd_index < 3)
	{
		strcpy(&input[0], argv[1]); 
		bzero(filename, 200);
		strncpy(filename, argv[0], strlen(argv[0]));
	}
}

/*
 * brief: sends file data packet by packet
 * fixed size of 100bytes is used 
 */
void reliable_put()
{	
    char *str = NULL;
    char databuf[DATABUFSIZE] = {0};
    int ret = 0;
	char packet = '0';
	int prv_bytes = 0;
    
	printf("Sending file %s\n", &filename[0]);
	
	fd = open(&filename[0], O_RDWR, S_IRWXU);
	if (fd == -1)
		{
        	error("ERROR opening file");
		 	return;
		}
			
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
			n = sendto(sockfd, "done", 5, 0, (struct sockaddr *) &serveraddr, serverlen);
			if (n < 0) 
			  error("ERROR in sendto");
			break;
		}

		// second byte - number of bytes in packet for verification at other end for missing data
		databuf[1] = ret;

		//send 1st-Packet, 2nd-Bytes, 3rd to 102nd - data		
		n = sendto(sockfd, databuf, ret+2, 0, (struct sockaddr *) &serveraddr, serverlen);
		if (n < 0) 
		  error("ERROR in sendto");

		//clear buf before recieving ACK		
		bzero(databuf, DATABUFSIZE);
		/* print the server's reply */
		n = recvfrom(sockfd, databuf, 6, 0, (struct sockaddr *) &serveraddr, &serverlen);
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
 * brief: receives file data packet by packet
 * fixed size of 100bytes is used per packet 
 */
void reliable_get()
{	
    char *str = NULL;
    char databuf[DATABUFSIZE] = {0};
    int bytes = 0;
	char packet = 0;
	uint32_t parity = 0;
		
	printf("Receiving file %s\n", &filename[0]);
	
	// create or open file of given name
	fd = open(&filename[0], O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
	if (fd == -1)
		{
			error("ERROR opening file");
		 	return;
		}
		
	while(1)
	{
		bzero(databuf, DATABUFSIZE);
		str = &databuf[0];
		
		//receive packet from server
		n = recvfrom(sockfd, databuf, DATABUFSIZE, 0,
		 	(struct sockaddr *) &serveraddr, &serverlen);
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
			n = sendto(sockfd, databuf, strlen(databuf), 0, (struct sockaddr *) &serveraddr, serverlen);
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
		n = sendto(sockfd, databuf, strlen(databuf), 0, (struct sockaddr *) &serveraddr, serverlen);
		if (n < 0) 
		  error("ERROR in sendto");
		
		//printf("Packet %c received successfully\n", packet);
	}

	//close file before stopping 'get' operation
	close(fd);
	printf("Received file %s successfully\n", &filename[0]);
}

/*
 * brief: main socket communication
 * client code to execute put, get, delete, ls. exit operations
 */
int main(int argc, char **argv) 
{
    int portno;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    while(1)
    {
		bzero(buf, BUFSIZE);
		printf("Command information:\n");
		printf("1. put <file name> : to send file to server\n");
		printf("2. get <file name> : to get file from server\n");
		printf("3. delete <file name> : delete's file at server\n");
		printf("4. ls : lists folder contents from server side\n");
		printf("5. exit: to exit server and client connection gracefully\n"); 
		printf("Please enter msg to be send from above: ");
		fgets(buf, BUFSIZE, stdin);
		
		printf("%s\n", buf);
		/* send the user message to the server */
		serverlen = sizeof(serveraddr);
		n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, serverlen);
		if (n < 0) 
		  error("ERROR in sendto");

		// process command typed by user  
		process_command(buf);
			
		bzero(buf, BUFSIZE);

		/* get the server's reply */
		n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
		if (n < 0) 
		  error("ERROR in recvfrom");
				
		// operation as per command
		switch (cmd_index)
		{
			case CMD_PUT:
				printf("Put file response from server: %s\n", buf);
				
				if((strncasecmp("File created", buf, strlen(buf))) != 0)
					{
		  				error("ERROR in Put file response from server");
		  				break;
					}
					
				reliable_put();				
			break;
			
			case CMD_GET:
				printf("Get file response from server: %s\n", buf);
				
				if((strncasecmp("File found", buf, strlen(buf))) != 0)
					{
		  				printf("ERROR in Put file response from server");
		  				break;
					}
				
				reliable_get();	
			break;
			
			case CMD_DEL:
				printf("deleting file from server: %s\n", buf);
			break;
			
			case CMD_LS:
				printf("ls data from server: \n%s\n", buf);
			break;
			
			case CMD_EXIT:				
				printf("Exit status from server: %s\n", buf);
				closegrace = true;
			break;
			
			default:
				printf("Echo from server: %s\n", buf);
			break;
		}

		// on 'exit' command break loop and close connection
		if (closegrace)
		{
			printf("Client exiting\n");
			close(sockfd);
			break;
		}
    }
    return 0;
}





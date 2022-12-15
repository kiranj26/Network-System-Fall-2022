/**************************************************/
/**************************************************/
/*     Lab 3  : Network Systems                   */
/*     Author : Kiran Jojare                      */
/*     Topic  : C Web Proxy                       */
/**************************************************/
/**************************************************/

/* Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>           
#include <strings.h>          
#include <unistd.h>          
#include <sys/socket.h>      
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <openssl/md5.h>
#include <sys/time.h>

/* Pthread Initialisation*/
pthread_t       tid;
pthread_mutex_t dns_lock;     /* Mutex for thread syncronisation */
pthread_mutex_t cache_lock;   /* Mutex for thread syncronisation */

/* Global Defines */
#define MAXLINE  8192         /* max text line length */
#define MAXBUF   8192         /* max I/O buffer size */
#define LISTENQ  1024         /* second argument to listen() */

/* Global Constants */
char    cacheDNSBuffer[MAXLINE];
int     timeout;

/* Function Declarations */
int     open_listening_socket(int port);
void *  thread_main(void * vargp);
void    throw_error(int connfd, char* msg);
int     check_if_blacklisted(char* hostname, char* ip);
int     check_dns_cache(char* hostname, struct in_addr* cache_addr);
int     add_ip_to_cache(char* hostname, char* ip);
void    md5_str(char* str, char* md5buf);
int     check_cache_md5(char* fname);
void    send_file_from_cache(int connfd, char* fname);
void    echo_http_request_response(int connfd);

/* **************************** */
/*          Main                */
/* **************************** */

int main(int argc, char **argv)
{
    /* Checking use of command line arguments */
    if (argc == 2){
      /* If no timeout provided keep default timeout value of 0 Sec */
      timeout = 0;
    } else 
        if (argc == 3){
          /* Timeout provided from command line argument */
          timeout = atoi(argv[2]);
    } else {
       printf("usage: %s <port>\nor: %s <port> <timeout>\n", argv[0], argv[0]);
	     exit(0);
    }

    int listen_fd, *conn_fdp, port;
    
    int client_len=sizeof(struct sockaddr_in);
    struct sockaddr_in client_addr;

    memset(cacheDNSBuffer, 0, sizeof(cacheDNSBuffer));

    /* Print time out and port number */
    port = atoi(argv[1]);
    printf("Timeout Selected \t = %d Sec \n",timeout);
    printf("Port Selected \t\t = %d \n",port);

    pthread_mutex_init(&dns_lock, NULL);
    pthread_mutex_init(&cache_lock, NULL);

    listen_fd = open_listening_socket(port);

    /* Continuesly check for connection request */
    while (1) {
	       conn_fdp = malloc(sizeof(int));

         /* Accept a connection on out listenning socket and create a new connected socket */
	       *conn_fdp = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);

	      /* Pass the connected socket after connection is established into a seperate thread function */  
        pthread_create(&tid, NULL, thread_main, conn_fdp);
    }
} /* End Main */

/* **************************** */
/*          End Main            */
/* **************************** */

/*
 * Function opens a listenning socket 
 * and returns the ID of that socket
 * Returns -1 in case of failure
 */
int open_listening_socket(int port)
{
    int listen_fd, optval=1;
    struct sockaddr_in server_addr;

    /* Creating an endpoint for communication using socket(2) */
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Set socket option to eliminate error "Address already in use" from bind. */
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* Building up server address configurations */
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons((unsigned short)port);

    /* Binding socket to the server address */
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        return -1;

    /* Make socket ready to accept incoming connection request using listen(3) */
    if (listen(listen_fd, LISTENQ) < 0)
        return -1;
    return listen_fd;
} 
/* end open_listening_socket */

/* Thread routine 
*  Thread main function  
*/
void * thread_main(void * vargp)
{
    int connfd = *((int *)vargp);         /* fetching argument passed to thread */ 
    pthread_detach(pthread_self());       /* mark thread as detached */ 
    free(vargp);
    echo_http_request_response(connfd);                         
    close(connfd);
    return NULL;
}/* end thread_main*/


void echo_http_request_response(int connfd)
{
    size_t  n;
    char    buf[MAXLINE];

    struct hostent* server;
    int     sock_fd;
    int     conn_size;
    int     portno = 80;
    printf("\n####################################################################################\n");
    /* read from connected socket and load data into buffer */
    n = read(connfd, buf, MAXLINE);
    printf("\nServer received a request ..........................................................\n\n");

    /* Parsing HTTP request from client */
    char * http_request = strtok(buf, " ");       // GET or POST
    char * http_hostname = strtok(NULL, " ");
    char * http_version = strtok(NULL, "\r");     // HTTP/1.1 or HTTP/1.0
    char file_name[MAXLINE];

    printf("Request type\t: %s\nHostname\t: %s\nVersion\t\t: %s\n\n", http_request, http_hostname, http_version);

    /* Error Detection */
    if(http_hostname == NULL || http_version == NULL){
      printf("Hostname Version not provided or maybe NULL\n");
      throw_error(connfd, "500 Internal Server Error");
      return;
    }

    if(strlen(http_hostname) == 0){
      printf("No host requested, responding with error\n");
      throw_error(connfd, "500 Internal Server Error");
      return;
    }

    if(!strcmp(http_version, "HTTP/1.1") || !strcmp(http_version, "HTTP/1.0")){
    } else {
      printf("Invalid HTTP version: %s\n", http_version);
      throw_error(connfd, "500 Internal Server Error");
      return;
    }

    if(!strcmp(http_request, "GET")){
    } else {
      printf("Invalid HTTP request: %s\n", http_request);
      throw_error(connfd, "400 Bad Request");
      return;
    }

    /* Strip HTTP:// from hostname to eliminate the error from gethostbyname */
    char* double_Slash = strstr(http_hostname, "//");
    if(double_Slash != NULL){
        http_hostname = double_Slash+2;
    }

    /* seperate host name and file name from HTTP Request */
    char* Slash = strchr(http_hostname, '/');
    if(Slash == NULL || *(Slash+1) == '\0'){ //If no file is explicitly requested, get default
      printf("Default page requested.............................................................\n\n");
      strcpy(file_name, "index.html");
    }

    else { //Otherwise, copy requested filename to buffer
      strcpy(file_name, Slash+1);
    }

    printf("Host\t\t: %s\nFile\t\t: %s\n", http_hostname, file_name);

    //Set string to end after hostname so the file is not part of DNS query
    if(Slash != NULL){
      *Slash = '\0'; 
    }

    /* Store MD5 Input for example netsys.cs.colorado.edu/index.html*/
    char MD5_inp[strlen(http_hostname)+strlen(file_name) + 2]; /* Two extra bytes for "/" and Null terminator "\0"*/
    strcpy(MD5_inp, http_hostname);
    strcat(MD5_inp, "/");
    strcat(MD5_inp, file_name);

    /* Store MD5 Output */
    /* 16 Byte Hex Value + 2 characters for int + Null terminator i.e 33 Bytes in total */
    char MD5_out[33];
    
    memset(MD5_out, 0, sizeof(MD5_out));

    printf("Hashing \t: %s\n", MD5_inp);
    md5_str(MD5_inp, MD5_out);

    /* Store Cache file as example "cache/8ef7ececfe8528bffb1d8ae1f639ce16" */
    char cache_buf[strlen("cache/") + strlen(MD5_out)]; /* Buffer file directory with relative path */
    strcpy(cache_buf, "cache/");
    strcat(cache_buf, MD5_out);

    if (check_cache_md5(MD5_out)){
      printf("Sending file %s\n", cache_buf);
      send_file_from_cache(connfd, cache_buf);
      return;
    }

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0){
      printf("Faild to open socket");
    }

    struct sockaddr_in server_addr;

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);


    struct in_addr cache_addr;
    int server_addr_cache = check_dns_cache(http_hostname, &cache_addr);
    if(server_addr_cache == -1){ 
      printf("Host %s not in DNS cache\n", http_hostname);
      server = gethostbyname(http_hostname);
      if(server == NULL){
        printf("Failed to resolve host %s, responding with 404 error\n", http_hostname);
        throw_error(connfd, "404 Not Found");
        return;
      }
      bcopy((char *)server->h_addr,
      (char *)&server_addr.sin_addr.s_addr, server->h_length);
    } else { //in cache
      server_addr.sin_addr.s_addr = cache_addr.s_addr;
    }

    /* Converting hostname to its IP using inet_ntop() */
    char IP_Addr_buf[20];
    if (inet_ntop(AF_INET, (char *)&server_addr.sin_addr.s_addr, IP_Addr_buf, (socklen_t)20) == NULL){
      printf("Failed to convert hostname to IP\n");
      return;
    }

    /* If the current hostname and IP is not in cache create a new entry for it */
    if(server_addr_cache == -1){ 
      /* Load IP of hostname into ./cache */
      int success = add_ip_to_cache(http_hostname, IP_Addr_buf);
      /* No more space in ./cache cant create entry */
      if(success == -1){
        printf("Cache full, cannot add entry %s:%s\n", http_hostname, IP_Addr_buf);
      }
    }
    printf("Host: %s, IP: %s\n", http_hostname, IP_Addr_buf);

    /* Checking for the Blacklist for given IP and hostname */
    printf("\nChecking Blacklist.................................................................\n");
    if(check_if_blacklisted(http_hostname, IP_Addr_buf)){
      /* HTTP 403 Forbidden Error thrown */
      throw_error(connfd, "403 Forbidden");
      return;
    }

    int server_len = sizeof(server_addr);

    conn_size = connect(sock_fd, (struct sockaddr*)&server_addr, server_len);
    if (conn_size < 0){
      printf("Failed to connect\n");
      return;
    }

    memset(buf, 0, MAXLINE);
    sprintf(buf, "GET /%s %s\r\nHost: %s\r\n\r\n", file_name, http_version, http_hostname);

    conn_size = write(sock_fd, buf, sizeof(buf));
    if (conn_size < 0){
        printf("Failed to sendto\n");
        return;
    }

    int total_size = 0;
    memset(buf, 0, sizeof(buf));

    FILE* file;
    file = fopen(cache_buf, "wb");
    pthread_mutex_lock(&cache_lock);
    while((conn_size = read(sock_fd, buf, sizeof(buf)))> 0){
      if (conn_size < 0){
          printf("Failed to recvfrom\n");
          return;
      }

      total_size += conn_size;

      write(connfd, buf, conn_size);
      fwrite(buf, 1, conn_size, file);
      memset(buf, 0, sizeof(buf));
    }
    pthread_mutex_unlock(&cache_lock);
    fclose(file);
    if(conn_size == -1){
      printf("Failed to read - errno:%d\n", errno);
      return;
    }
    printf("Total %d bytes recieved ........................................................\n", total_size);
} /* end echo_http_request_response */


/* Function checks if the MD5 hash file is available in the cache
*  Returns 1 if the Chache is available and withing timeout
*  Returns 0 if the Cache is available and not withing timeout or not available in cache
*  It also creates a cache flder if not available in directory
*/
int check_cache_md5(char* fname){
  struct stat fileStat;
  DIR* dir = opendir("./cache");

  //sizeof("cache") includes null terminator, but it will be replace with '/'
  char buf[strlen("cache/") + strlen(fname)];
  memset(buf, 0, sizeof(buf));
  strcpy(buf, "cache/");
  strcat(buf, fname);

  /* Searching file in ./cache */
  printf("\nSearching for file %s ..........................\n\n", buf);
  printf("Response via Caching from Software ......\n");
  if(dir){
    closedir(dir);
    if(stat(buf, &fileStat) != 0){
      printf("File not in cache\n");
      return 0;
    }
    printf("File located in cache folder, Figuring out for timeout...\n");
    if(timeout == 0){
      printf("Timeout value not given\n");
      return 1;
    }
    /* Checking timeout expired or not  */
    time_t time_file_modify = fileStat.st_mtime;
    time_t time_current = time(NULL);
    double time_diff = difftime(time_current, time_file_modify);
    if(time_diff > timeout){
      printf("Timeout occurred: file has been modified %.2f seconds ago, Given timeout is %d\n", time_diff, timeout);
      return 0;
    }
    printf("File is cachable for %d more seconds since timeout is yet to elapse\n", timeout - (int)time_diff);
    return 1;
  }
  /* If folder ./cache doesnt exits create one */
  else if(errno = ENOENT){
    printf("Cache folder not found, creating...\n");
    mkdir("cache", 0777);
    printf("Created directory ./cache \n");
    closedir(dir);
    return 0;
  }
  else {
    printf("Failed to open cache folder\n");
    return 0;
  }
}/* end check_cache_md5*/


/* Function sends cache file from ./cache 
 * to client connected socket 
*/
void send_file_from_cache(int connfd, char* fname){
  FILE* file = fopen(fname, "rb");
  if(!file){
    printf("Fail to open file %s\n", fname);
    return;
  }
  fseek(file, 0L, SEEK_END);
  int fsize = ftell(file);
  rewind(file);
  char file_buf[fsize];
  fread(file_buf, 1, fsize, file);
  write(connfd, file_buf, fsize);
}/* end send_file_from_cache*/

/* Function sends the error message back to server */
void throw_error(int connfd, char* msg)
{
  char error_msg[MAXLINE];
  sprintf(error_msg, "HTTP/1.1 %s\r\nContent-Type:text/plain\r\nContent-Length:0\r\n\r\n", msg);
  write(connfd, error_msg, strlen(error_msg));
}/* end throw_error */

/* Function checks if the given IP of 
 * hostname is blacklisted as per blacklist.txt
 */
int check_if_blacklisted(char* hostname, char* ip){
  FILE *   file;
  char     blacklist_line[100];
  char *   newline;

  if(access("blacklist", F_OK) == -1){
    printf("No blacklist named blacklist.txt found\n");
    return 0;
  }
  file = fopen("blacklist", "r");
  while(fgets(blacklist_line, sizeof(blacklist_line), file)){
    newline = strchr(blacklist_line, '\n');
    if(newline != NULL){
      *newline = '\0';
    }
    /* Checking in blacklist */
    if(strstr(blacklist_line, hostname) || strstr(blacklist_line, ip)){
      printf("Blacklist hostname found: %s\n", blacklist_line);
      return 1;
    }
  }
  fclose(file);
  printf("Hostname not found in blacklist\n");
  return 0;
} /* end check_if_blacklisted */

/* Function check_dns_cache checks for the DNS cache 
 * If hostname is resolved earlier function returns 1 
 * If not returns -1
*/
int check_dns_cache(char* hostname, struct in_addr* cache_addr){
  printf("Checking for %s in cache!!!\n", hostname);

  char* cache_line;
  char* temp_buf = calloc(strlen(cacheDNSBuffer)+1, sizeof(char));
  strcpy(temp_buf, cacheDNSBuffer); 

  char* hostname_match = strstr(temp_buf, hostname);
  /* If hostname missing from cache */
  if(hostname_match == NULL){ 
    return -1;
  }

  cache_line = strtok(hostname_match, ":");
  cache_line = strtok(NULL, "\n");
  printf("Found DNS cache entry %s:%s\n", hostname, cache_line);
  inet_pton(AF_INET, cache_line, cache_addr);
  free(temp_buf);
}/* end check_dns_cache */

/* Adds specified hostname, ip pair to dns cache */
int add_ip_to_cache(char* hostname, char* ip){

  pthread_mutex_lock(&dns_lock);

  char* entry = strrchr(cacheDNSBuffer, '\n');
  char  buf[100];
  
  memset(buf, 0, sizeof(buf));
  snprintf(buf, 100, "%s:%s\n", hostname, ip);
  
  if(entry == NULL){
    printf("Cache empty\n");
    strncpy(cacheDNSBuffer, buf, strlen(buf));
    pthread_mutex_unlock(&dns_lock);
    return 0;
  }
  /* Cache Full */
  if(entry + strlen(buf)+1 > cacheDNSBuffer+sizeof(cacheDNSBuffer)){ 
    return -1;
    pthread_mutex_unlock(&dns_lock);
  }
  strncpy(entry+1, buf, strlen(buf));
  pthread_mutex_unlock(&dns_lock);
} /* end add_ip_to_cache */

//Implementation from https://stackoverflow.com/questions/7627723/how-to-create-a-md5-hash-of-a-string-in-c
void md5_str(char* str, char* md5buf){
  unsigned char MS5_sum[16];
  MD5_CTX       context;

  MD5_Init(&context);
  MD5_Update(&context, str, strlen(str));
  MD5_Final(MS5_sum, &context);

  for(int i = 0; i < 16; ++i){
    sprintf(md5buf+i*2, "%02x", (unsigned int)MS5_sum[i]);
  }
  printf("Final hash\t: %s -> %s\n", str, md5buf);
}/* end md5_str */

/* **************************** */
/*            End               */
/* **************************** */
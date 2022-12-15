/* 
Programming Assignment 2 : 
Name                     : Kiran Jojare 
Course                   : Network System
Semester                 : Fall 2022
*/

/* Include Files */
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<errno.h>
#include<libgen.h>
#include<pthread.h>


/* Definations */
#define MAX_CONNECTION 32
#define BUFFLENGTH 1024
#define HTTP_REQ_BUFFER_SIZE 50


/* struct for thread data handling */
typedef struct
{ int s_port;                               // server port number
  char doc_root[200];                       // document root directory
  char content_type[200];                   // content type
}Config_t;                                  // 

typedef struct
{ int id;                                   // client socket ID
  Config_t sconf;
}ThreadArgs_t;                              // structure for thread arguments

/* Function Declaration */ 
void *Thread_Main(void * args);             // thread function 
int Process_Conf_File(Config_t *sc_ptr);    // function to process settigs.conf file



/*******************************************/
/********** Main Function ******************/
/*******************************************/
int main(int argc, char **argv)
{
  // exit if args used incorrectly and show correct usage
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
  pthread_t th;                             // thread argument
  pthread_attr_t attr;                      // thread attribute argument

  ThreadArgs_t args;                        // thread data as structure argument

  int s_sock;                               // server socket ID
  int c_sock;                               // client socket ID

  struct sockaddr_in s_addr;                // server address
  struct sockaddr_in c_addr;                // client address

  // initialising thread attributes
  if (pthread_attr_init(&attr) != 0) {
      perror("\nInit Attr Failed\n");
      exit(1);
  }

  // settig detached state attributes
  if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
      perror("\nPthread Attribute State Set Failed\n");
      exit(1);
  }

  s_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(s_sock == -1){
    perror("\nSocket Creation Failed\n");
  }

  // Load, process and print the data from settings.config
  if(Process_Conf_File(&args.sconf) == -1){
    printf("\n Failed to load config file settings.conf \n");
    exit(1);
  } else
  { printf("\n(-) Loading Configuration File \t: settings.conf \n");
    printf("(-) Server Port Number \t\t: %d\n", args.sconf.s_port);
    printf("(-) Document Root \t\t: %s\t\n", args.sconf.doc_root);
    printf("(-) Content type \t\t: %s\n\n", args.sconf.content_type);
  }

  // server address attributes
  s_addr.sin_family = AF_INET;
  s_addr.sin_addr.s_addr = INADDR_ANY;
  s_addr.sin_port = htons(args.sconf.s_port);

  // setting up server socket for connection
  printf("Server Started.....\n");
  if(bind(s_sock, (struct sockaddr *) &s_addr, sizeof(s_addr)) < 0)
  {
    perror("\nServer Bind Failed ");
    return 1;
  }else{ printf("Server Bind Successfull.....\n");
  }

  if(listen(s_sock, MAX_CONNECTION) == -1)
  {
    perror("\nServer Listen failed");
  } else{ printf("Server Listen Successfull.....\n");
  }
  // start listening to client connction using while loop
  while (1)
  {
    int cl_adlen = sizeof(c_addr);
    // accept first client
    c_sock = accept(s_sock, (struct sockaddr *)&c_addr, &cl_adlen); 
    if(c_sock < 0)
    { perror("\nError Creating Connected Socket\nClient Accept Failed\n");              
      continue;                   // connecting to server socket failed
    }
    else
    {
      args.id = c_sock;           // connect socket ID assigned as thread argument th.id
      printf("\nConncting Socket Created with ID = %d\n",args.id);
      if(pthread_create(&th, &attr, &Thread_Main, (void *)&args) != 0)
      { perror("\nThread Creation Failed: ");
      }
      printf("\nThread created for c_sock %d on port %d\n", args.id, ntohs(c_addr.sin_port));
    }
  }
  pthread_attr_destroy(&attr);    // thread attributes cleared
  close(s_sock);                  // server socket closed
  return 1;
}

/**************************************************/
/************ Function Definations  ***************/
/**************************************************/

/* Function to read config file */
int Process_Conf_File(Config_t *sc_ptr)
{
  size_t len = 0;
  ssize_t char_read;                                         // variable to store number of char read from config file

  char *line = NULL;                                         // line pointer assigned NULL for getline function
  bzero(sc_ptr->doc_root, sizeof(sc_ptr->doc_root));         

  FILE *conf_file;                                           // opened file descriptor

  conf_file = fopen("settings.conf", "r");
  if(conf_file!=NULL)                                        // file open option was successfull
  {
    // reading an entire file line by line from conf file returns no of char read into char_read
    while((char_read = getline(&line, &len, conf_file)) != -1)     
    {
      if (strstr(line, "Listen") != NULL)
      {
        sc_ptr->s_port = atoi(line+6);                        // loading server port from file
      }
      if (strstr(line, "DocumentRoot") != NULL)
      {
        strncpy(sc_ptr->doc_root, line+14, strlen(line+16));
        sc_ptr->doc_root[strlen(sc_ptr->doc_root)] = '\0';    // loading doc root from file
      }
      if (line[0] == '.')
      {
        strcat(sc_ptr->content_type, line);
      }
    }
    fclose(conf_file);
  }
  else
  {
    return -1;                                                // file open option failed
  }
  return 0;
}

/* Function for thread activity */
void *Thread_Main(void * args)
{
  // loading thread arguments into thead data structure
  ThreadArgs_t thread_data = *(ThreadArgs_t*) args;
  
  // initialisiing arrays for HTTP msg storage
  char buffer_in[BUFFLENGTH];
  char buffer_out[BUFFLENGTH];
  char http_req_type[HTTP_REQ_BUFFER_SIZE];
  char http_req_path[HTTP_REQ_BUFFER_SIZE];
  char http_req_version[HTTP_REQ_BUFFER_SIZE];

  char path[250];
  char ext[10];
  char cont_len[10];

  char * http_req_copy;
  char * extension;
  char * con_type;
  char * line;
  char * read;

  int recv_bytes;
  int read_len;

  size_t file_size;
  // default format for HTTP response OK
  char http_response_ok[100] = "HTTP/1.1 200 Document Follows\nContent-Type:";

  // load connecting socket ID for the thread or exit the thread
  int conn_sock = thread_data.id;
  if(conn_sock==0)
  { perror("\nFailed to Load Connected Socket\n");
    close(conn_sock); pthread_exit(NULL);
  }
  printf("Socket Id Passed To Thread\t\t: %d\n", conn_sock);


  bzero(buffer_in, sizeof(buffer_in));
  strcpy(path, thread_data.sconf.doc_root);
  printf("Doc Root Passed To Thread\t\t: %s\n", path);

  // recieve data from connecting socket into input buffer
  recv_bytes = recv(conn_sock, buffer_in, sizeof(buffer_in), 0);
  if (recv_bytes < 1)
  { printf("\n\nReceive from Conecting Socket Failed\nSocket [%d] closed\n",conn_sock);
    close(conn_sock);                                          // exit thread if data recieved failed on connecting socket
    pthread_exit(NULL);                                 
  }

  // load entire http command into input buffer
  char http_reqst[BUFFLENGTH];                                 // char array to store complete HTTP request                      
  strcpy(http_reqst, buffer_in);                               // load HTTP request received from buffer to array
  http_req_copy = strdup(http_reqst);                          // create duplicate copy of HTTP request

  // load subsequent part of HTTP command into req type, path and version
  // strtok_r will extract type path and version from HTTP request
  strcpy(http_req_type, strtok_r(http_req_copy, " ", &http_req_copy));
  strcpy(http_req_path, strtok_r(http_req_copy, " ", &http_req_copy));
  strcpy(http_req_version, strtok_r(http_req_copy, "\n", &http_req_copy));

  if(strstr(http_reqst, "keep-alive")!=NULL)
  {
    printf("Keep Alive Detected\n");
  }
  printf("Request Type\t: %s\n", http_req_type);
  printf("Request Path\t: %s\n", http_req_path);
  printf("Request version\t: %s\n", http_req_version);

  FILE *file_ptr;
  /* Process HTTP request */
  if (!strcmp(http_req_type, "GET"))
  {
    printf("Processing the GET request\n");
    if (!strcmp(http_req_path, "/"))
    {
      strcat(path, "/index.html");
      file_ptr = fopen(path, "r");
      if (file_ptr==NULL)
      {
        // if /index.html path fails to open and HTTP request path is invalid
        // throw 404 Not Found Reason URL does not exist and exit the thread
        strcpy(buffer_out, "HTTP/1.1 404 Not Found\nContent-Type:text/html\n\n");
        send(conn_sock, buffer_out, strlen(buffer_out), 0);
        bzero(buffer_out, sizeof(buffer_out));
        strcpy(buffer_out, "<html><body>404 Not Found Reason URL does not exist :<<requested url>></body></html>");
        send(conn_sock, buffer_out, strlen(buffer_out), 0);
        bzero(buffer_out, sizeof(buffer_out));
        printf("Socket [%d] closed\n", conn_sock);
        close(conn_sock);
        pthread_exit(NULL);

      }
      else
      {
        con_type = "text/html";
      }
    }
    else
    {
      // if HTTP request path is valid
      strcat(path, http_req_path);
      file_ptr = fopen(path, "r");
      if (file_ptr==NULL)
      {
        perror("\nFile open error\t Failed to Open HTTP Request Path\n");
        strcpy(buffer_out, "HTTP/1.1 404 Not Found\nContent-Type:text/html\n\n");
        send(conn_sock, buffer_out, strlen(buffer_out), 0);
        bzero(buffer_out, sizeof(buffer_out));
        strcpy(buffer_out, "<html><body>404 Not Found Reason URL does not exist :<<requested url>></body></html>");
        send(conn_sock, buffer_out, strlen(buffer_out), 0);
        printf("Response: %s\n", buffer_out);
        close(conn_sock);
        pthread_exit(NULL);
      }
      else
      {
        read = strdup(thread_data.sconf.content_type);            // load content type into a char array
        while(line = strtok_r(read, "\n", &read))                 // strtok_r parses through the read till \n is encourtered
        {
          extension = strtok_r(line, " ", &line);                 // parses till sppace
          if(strstr(path, extension) != NULL)
          {
            con_type = strtok_r(line, "\n", &line);               // parses till \n encountered
            break;
          }
        }
        if(con_type==NULL)
        {
          close(conn_sock);
          pthread_exit(NULL);
        }
      }
    }

  printf("Path: %s\n", path);
  /* fseek and ftell the HTTP request path */
  // file pointer is http request path
  // using fseek to seek to the end of file
  fseek(file_ptr, 0, SEEK_END);
  // ftell points to the current value of the 
  // stream pointed by file pointer
  file_size = ftell(file_ptr);  // get current file pointer
  fseek(file_ptr, 0, SEEK_SET); 

  sprintf(cont_len, "%lu", file_size);
  strcat(http_response_ok, con_type);
  strcat(http_response_ok, "\nContent-Length: ");
  strcat(http_response_ok, cont_len);
  strcat(http_response_ok, "\n\n");
  printf("Response: %s\n", http_response_ok);
  strcpy(buffer_out, http_response_ok);
  send(conn_sock, buffer_out, strlen(buffer_out), 0);
  bzero(buffer_out, sizeof(buffer_out));
  
  // read file data upto BUFFLENGTH count bytes
  do {
    read_len = fread(buffer_out, 1, BUFFLENGTH, file_ptr);
    send(conn_sock, buffer_out, sizeof(buffer_out), 0);
    bzero(buffer_out, sizeof(buffer_out));
  } while (read_len==BUFFLENGTH);
  
  bzero(path, sizeof(path));
  fclose(file_ptr);
  }

  // Processing HTTP POST Request type
  else if ((!strcmp(http_req_type, "POST"))||(!strcmp(http_req_type, "DELETE"))||(!strcmp(http_req_type, "OPTIONS"))||(!strcmp(http_req_type, "HEAD")))
  {
    //if (!strcmp(http_req_type, "POST"))
    //{
      printf("Processing the POST request\n");
      if (!strcmp(http_req_path, "/"))
      {
        strcat(path, "/index.html");
        file_ptr = fopen(path, "r");
        if (file_ptr==NULL)
        {
          // procssing index.html failure 
          strcpy(buffer_out, "HTTP/1.1 404 Not Found\nContent-Type:text/html\n\n");
          send(conn_sock, buffer_out, strlen(buffer_out), 0);
          bzero(buffer_out, sizeof(buffer_out));
          strcpy(buffer_out, "<html><body>404 Not Found Reason URL does not exist :<<requested url>></body></html>");
          send(conn_sock, buffer_out, strlen(buffer_out), 0);
          bzero(buffer_out, sizeof(buffer_out));
          printf("Socket [%d] closed\n", conn_sock);
          close(conn_sock);
          pthread_exit(NULL);
        }
        else
        {
          con_type = "text/html";
        }
      }
      else
      {
        strcat(path, http_req_path);
        file_ptr = fopen(path, "r");
        if (file_ptr==NULL)
        {
          perror("\nFile open error\t Failed to Open HTTP Request Path\n");
          strcpy(buffer_out, "HTTP/1.1 404 Not Found\nContent-Type:text/html\n\n");
          send(conn_sock, buffer_out, strlen(buffer_out), 0);
          bzero(buffer_out, sizeof(buffer_out));
          strcpy(buffer_out, "<html><body>404 Not Found Reason URL does not exist :<<requested url>></body></html>");
          send(conn_sock, buffer_out, strlen(buffer_out), 0);
          printf("Response: %s\n", buffer_out);
          close(conn_sock);
          pthread_exit(NULL);
        }
        else
        {
          read = strdup(thread_data.sconf.content_type);
          while(line = strtok_r(read, "\n", &read))
          {
            extension = strtok_r(line, " ", &line);
            if(strstr(path, extension) != NULL)
            {
              con_type = strtok_r(line, "\n", &line);
              break;
            }
          }
          if(con_type==NULL)
          {
            close(conn_sock);
            pthread_exit(NULL);
          }
        }
      }

    printf("Path: %s\n", path);

    fseek(file_ptr, 0, SEEK_END); // seek to end of file
    file_size = ftell(file_ptr);  // get current file pointer
    
    fseek(file_ptr, 0, SEEK_SET); 
    sprintf(cont_len, "%lu", file_size);
    strcat(http_response_ok, con_type);
    strcat(http_response_ok, "\nContent-Length: ");
    strcat(http_response_ok, cont_len);
    strcat(http_response_ok, "\n\n");
    printf("Response: %s\n", http_response_ok);
    strcpy(buffer_out, http_response_ok);
    send(conn_sock, buffer_out, strlen(buffer_out), 0);
    bzero(buffer_out, sizeof(buffer_out));
    
    do {
      read_len = fread(buffer_out, 1, BUFFLENGTH, file_ptr);
      send(conn_sock, buffer_out, sizeof(buffer_out), 0);
      bzero(buffer_out, sizeof(buffer_out));
    } while (read_len==BUFFLENGTH);
    
    bzero(path, sizeof(path));
    fclose(file_ptr);
    //}
    printf("Socket [%d] closed\n", conn_sock);
    close(conn_sock);
    pthread_exit(NULL);
  }
  else
  {
    // Processing HTTP 501 Failure request
    strcpy(buffer_out, "HTTP/1.1 501 Not Implemented\nContent-Type:text/html\n\n");
    send(conn_sock, buffer_out, strlen(buffer_out), 0);
    bzero(buffer_out, sizeof(buffer_out));
    strcpy(buffer_out, "<html><body501 Not Implemented <<error type>>: <<requested data>></body></html>");
    send(conn_sock, buffer_out, strlen(buffer_out), 0);
    bzero(buffer_out, sizeof(buffer_out));
    printf("Socket [%d] closed\n", conn_sock);
    close(conn_sock);
    pthread_exit(NULL);
  }

   printf("Socket %d Closed\n", conn_sock);
   close(conn_sock);
   pthread_exit(NULL);
}
/****************************************/
/*************** The End ****************/
/****************************************/
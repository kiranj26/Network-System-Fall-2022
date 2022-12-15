# Simple HTTP Based TCP Webserver
This project is the programming assignment 2 for CSCI-5273 Network Systems (Fall 2022) at CU Boulder.
The premise is to create a basic HTTP webserver that could respond to multiple connections simultaneously. 
The webserver has been implemented using `C Programming` with Multithreading approach i.e `pthreads` to handle multiple connections.


## Usage
In the root directory of the project is a Makefile. Simply running `make`. The usage of the program is as follows:

```shell script
./server <PORT>
```
Note : Run the server using Port Number 9001
```shell script
./server 9001
```
The server runs in an infinite loop until it receives the SIGINT signal via CTRL+C. The content it looks to serve should
be located at `./www`. Failure to create this directory with an "index.htm" or "index.html" file will cause the program
to immediately exit.


## Files of Importance
The following are explanations of the code files and their significance.

### server.c
The `server.c` is a C program that contains the main code for this project. It is thoroughly commented with adequately
named variables to assist in your understanding. Please reference the code for more information about this file. The code uses the pthread libraraies to handle each HTTP request. Both HTTP GET as well as HTTP POST requests are handled in program. The code loads a HTML wen server from file `settings.config` using function `Process_Conf_File`. Similarly to process and respond to HTTP request, code uses functiona named `Thread_Main`. 

### Makefile
The Makefile includes one make target to compile the server. Running `make` will produce the `./server` executable which is necessary to run the program.

### settings.config
The `settings.config` file stores listener port number, document root directory and content type to be supported by the web server. 
The default page cofiguration is also present in the config file. This config file is loaded by `server.c` using function `Process_Conf_File`.


## Limitations
Due to the constraints of the writeup, this server could not be designed to handle all file types that can be served
over a normal HTTP server. It is only built to handle the following file types:
* `.htm`
* `.html`
* `.txt`
* `.png`
* `.gif`
* `.jpg`
* `.ico`
* `.css`
* `.js`

Attempting to request a file type other than these will result in an HTTP 500 error.


## Testing
The server is build to handle multiple request at once using a parallel programming approach. The C programming based `pthread` libraries are used to deal with multiple connection. For each connection request the thread function is envoked.

The code has been tested using telnet applications. Program works fine for `HTPP GET` and `HTTP POST` requests. 


## Credits
This project was designed by Kiran Jojare at the University of Colorado Boulder during the Fall 2020 semester.
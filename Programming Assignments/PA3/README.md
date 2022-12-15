# Lab 3 : C Web Proxy
Student Name : Kiran Narendra Jojare 

## Summary:

Using this program, an HTTP forward web proxy is implemented. Your web browser and the proxy serve as a middleman between each other and the internet. It has the ability to locally cache files retrieved by HTTP requests so they do not have to be fetched again when a subsequent request is made. Files that were cached after the specified timeout value will not be served, which the user can supply as an option. Additionally, it has a DNS cache, so after resolving a hostname once, it won't need to be done again until the proxy is terminated. In the "blacklist" file, it also supports blacklisting by hostname or IP. Mutexes are used by the program to prevent shared thread caches from being overwritten (program memory DNS cache or local file cache). Be aware that the majority of contemporary websites will not function with this proxy because it only supports HTTP and not HTTPS.

## Prerequisite

### Configring using browser 
I used Firefox to test this proxy. Go to Preferences > Network Settings > Settings to configure Firefox to use a proxy.
In the "HTTP proxy" field, choose "Manual proxy configuration," and type "localhost". When running the proxy, pick a port and enter that port as a command-line argument. The browser is now set up to use the proxy after selecting "Use this proxy server for all protocols."

### Building proxy
There is an included makefile to make building the proxy easier. Simply run the
command "make" in the directory. This will generate a binary named "webproxy"

### Running the proxy
A port number and a timeout value for the local file cache are the only required and optional command line arguments for the server. The proxy will not serve files that were cached after the timeout value.

To run the proxy with no timeout, run the command
`./webproxy <port>`

To run the proxy with a timeout, run the command
`./webproxy <port> <timeout>`

## Functionality
* The proxy only supports HTTP requests. Functionality for HTTPS requests is unclear.
* The proxy forwards each and every `GET` request to the HTTP server. All other requests are rejected and given a failure. Local caching is used to store the HTTP response before sending it to the client. After cache timeout> seconds, the cached responses expire and must be retrieved again.
* All DNS query outcomes are locally cached. The local cache is checked to see if the hostname has already been resolved before executing a DNS query.
* Hostnames and IP addresses are separated by newlines in the file `blacklist.txt`. An error is returned if an HTTP request contains a hostname or IP address that has been blacklisted. When referencing the blacklist, hostnames are also converted to IP addresses when more than one hostname resolves to the same IP address.
* This implementation makes the assumption that there are an integer number of complete HTTP requests for each call to read.
* We didn't attempt the link prefetching
* Because setting up the proxy on Firefox is much simpler than on Chrome, we used Firefox for all testing; however, we don't understand why this proxy would not work in other browsers.
* Every HTTP result should contain a Content-Length: field; otherwise, we assume that the HTTP result has no content.

### Error codes
* `400`: DNS lookup couldn't resolve the hostname.
* `403`: The hostname is blacklisted.
* `405`: Method not supported (The method is not `GET`).

## Testing
We can test our proxy using `telnet` or `webbrowser`.
#### Option 1: telnet
		You can use telnet to test the functionality of the webserver
		1) telnet localhost <port> 
		   GET http://www.google.com HTTP/1.0
		   This should return the header files for google.com
#### Option 2: Browser
        You can use firefox or chome as long as the proxy server is 
		set up correctly on the browser
		1) Set up proxy on browser
		2) Run proxy on same port
		3) netsys.cs.colorado.edu
			You can use in browser inspection of network for 
			indepth information about each packet








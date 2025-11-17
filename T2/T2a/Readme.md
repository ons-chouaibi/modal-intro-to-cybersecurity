**wgetX - HTTP Client Implementation**
A simplified HTTP client that downloads web resources using TCP sockets. 

**Features**

URL parsing with protocol, hostname, port, and path identification
IPv4 and IPv6 support through getaddrinfo
HTTP request generation and sending
Dynamic buffer management for handling files of any size
HTTP header parsing

**Building**
Compile the program using the provided Makefile:
bashmake
To clean up the compiled files:
bashmake clean
Usage
bash./wgetX <URL> [output_filename]
Where:

<URL> is a valid HTTP URL in the format http://hostname[:port]/path
[output_filename] is an optional argument for the downloaded file name (defaults to "received_page")
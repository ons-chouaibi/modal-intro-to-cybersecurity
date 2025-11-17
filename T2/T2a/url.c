/**
 *  Jiazi Yi
 *
 * LIX, Ecole Polytechnique
 * jiazi.yi@polytechnique.edu
 *
 * Updated by Pierre Pfister
 *
 * Cisco Systems
 * ppfister@cisco.com
 *
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"url.h"

/**
 * parse a URL and store the information in info.
 * return 0 on success, or an integer on failure.
 *
 * Note that this function will heavily modify the 'url' string
 * by adding end-of-string delimiters '\0' into it, hence chunking it into smaller pieces.
 * In general this would be avoided, but it will be simpler this way.
 */

int parse_url(char* url, url_info *info)
{
	// url format: [http://]<hostname>[:<port>]/<path>
	// e.g. https://www.polytechnique.edu:80/index.php

	char *column_slash_slash, *host_name_path, *protocol, *path;
	int port;

	column_slash_slash = strstr(url, "://");
	// it searches :// in the string. if exists,
	//return the pointer to the first char of the match string

	if(column_slash_slash != NULL){ //protocol type exists
		*column_slash_slash = '\0'; //end of the protocol type
		host_name_path = column_slash_slash + 3; //jump to the host name
		protocol = url;
	} else {	//no protocol type: using http by default.
		host_name_path = url;
		protocol = "http";
	}


	/*
	 * To be completed:
	 * 	 store info->protocol
	 */

	info->protocol=protocol;

	/*
     * To be completed:
	 * 	 Return an error (PARSE_URL_PROTOCOL_UNKNOWN) if the protocol is not 'http' using strcmp.
	 */

	if ((strcmp(info->protocol,"http")!=0) && strcmp(info->protocol, "https") != 0){
		return 3;
	}

	/*
	 * To be completed:
	 * 	 Look for the first '/' in host_name_path using strchr
	 * 	 Return an error (PARSE_URL_NO_SLASH) if there is no such '/'
	 * 	 Store the hostname and the queried path in the structure.
	 *
	 * 	 Note: The path is stored WITHOUT the first '/'.
	 * 	       It simplifies this function, but I'll let you understand how. :-)
	 */
	path=strchr(host_name_path,'/');
	if (path==NULL){
		
		return 1;
	}
	*path = '\0';  
	info->host = host_name_path;
	info->path = path + 1;

	/*
	 * To be completed:
	 * 	 Look for ':' in the hostname
	 * 	 If ':' is not found, the port is 80 (store it)
	 * 	 If ':' is found, split the string and use sscanf to parse the port.
	 * 	 Return an error if the port is not a number, and store it otherwise.
	 */
	char *colon = strchr(info->host, ':');
	if (colon == NULL) {
		info->port = 80;  // Default HTTP port
	} else {
		*colon = '\0';  // End the hostname string
		char *port_str = colon + 1;
		if (sscanf(port_str, "%d", &port) != 1) {
			return 2;  // PARSE_URL_PORT_NOT_A_NUMBER
		}
		info->port = port;
	}

	// If everything went well, return 0.
	return 0;
}
void print_url_error(int error_code) {
    fprintf(stderr, "URL error: %s\n", parse_url_errstr[error_code]);
}
/**
 * print the url info to std output
 */
void print_url_info(url_info *info){
	printf("The URL contains following information: \n");
	printf("Protocol:\t%s\n", info->protocol);
	printf("Host name:\t%s\n", info->host);
	printf("Port No.:\t%d\n", info->port);
	printf("Path:\t\t/%s\n", info->path);
}

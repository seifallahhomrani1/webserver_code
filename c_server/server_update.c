// Server side C program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>


#define PORT 1234
#define WEBROOT "./webroot"

int recv_line(int sockfd, unsigned char *dest_buffer){
#define EOL "\r\n" // End-Of-line byte sequence
#define EOL_SIZE 2
		unsigned char *ptr;
		int eol_matched = 0;

		ptr = dest_buffer; 
		while(recv(sockfd, ptr,1,0) == 1){
			if (*ptr == EOL[eol_matched]){
				eol_matched++;
				if(eol_matched == EOL_SIZE){
					*(ptr+1-EOL_SIZE) = '\0'; 
					return strlen(dest_buffer);
				}
			} else {
				eol_matched = 0;
			}
			ptr++; // increment pointer to the next byter
		}
		return 0; // Didn't find the end-of-line characters
}



// send_string
int send_string(int sockfd, unsigned char *buffer) {
    int sent_bytes, bytes_to_send;
    bytes_to_send = strlen(buffer);
    while(bytes_to_send > 0) {
        sent_bytes = send(sockfd, buffer, bytes_to_send, 0);
        if(sent_bytes == -1)
            return 0; // Return 0 on send error.
		bytes_to_send -= sent_bytes;
        buffer += sent_bytes;
    }
    return 1; // Return 1 on success.
    }


int get_file_size(int fd) {
	struct stat stat_struct;
	if(fstat(fd, &stat_struct) == -1)
		return -1;
	return (int) stat_struct.st_size;
}		


void handle_connection(int sockfd, struct sockaddr_in *client_addr_ptr){

		unsigned char *ptr, request[500], resource[500]; 
		int fd, length; 

		//recv_line : 
		length = recv_line(sockfd, request);
	
        // inet_ntoa : (Network to ASCII) used to convert in_addr structure(32 bit integer representing the IP address) to a string containg an IP address in dotted format   
		printf("Got request from %s:%d \"%s\"\n", inet_ntoa(client_addr_ptr->sin_addr),ntohs(client_addr_ptr->sin_port), request); 
		
		ptr = strstr(request, " HTTP/"); // Check if it's an http request or not (strstr finds the first occurrence of the substring)
		if(ptr == NULL){
			printf("NOT HTTP!\n");
		}else{
			*ptr = '\0'; // Terminate the buffer at the end of the URL
			ptr = NULL; // Set ptr to NULL (used to flag for an invalid request) 
			if(strncmp(request, "GET ",4) == 0) //  compares at most the first n bytes of str1 and str2. - If true then its a GET request
				ptr = request+4; // ptr is the URL
			if (strncmp(request, "HEAD ",5) ==0) //
				ptr = request+5;


		if(ptr==NULL){ //Then this is not a recognized request
			printf("\tUNKNOWN REQUEST!\n");
		}else{ //Valid request with ptr pointing to the resource name
			if (ptr[strlen(ptr)-1]=='/')
				strcat(ptr,"index.html"WEBROOT);
            
            //constructing the resource :WEBROOT
			strcpy(resource,WEBROOT);
			strcat(resource,ptr);
            //printf("%s", resource);
			fd = open(resource, O_RDONLY,0); //Trying to open the file ( 0 stands for a file )
			printf("\tOpening \'%s\'\t",resource);
			if (fd == -1) { //If file is not found 
					printf("404 Not Found\n");
					send_string(sockfd, "HTTP/1.0 404 NOT FOUND\r\n");
					send_string(sockfd, "Server: Tiny webserver\r\n\r\n");
					send_string(sockfd, "<html><head><title>404 Not Found</title></head>");
					send_string(sockfd, "<body><h1>URL not found</h1></body></html>\r\n");
		} else {
		// Otherwise, serve up the file.


		printf(" 200 OK\n");
		send_string(sockfd, "HTTP/1.0 200 OK\r\n");
		send_string(sockfd, "Server: Tiny webserver\r\n\r\n");


		if(ptr == request + 4) { // Then this is a GET request
			if( (length = get_file_size(fd)) == -1)
				perror("getting resource file size");
			if( (ptr = (unsigned char *) malloc(length)) == NULL)
				perror("allocating memory for reading resource");
			read(fd, ptr, length); // Read the file into memory.
			send(sockfd, ptr, length, 0); // Send it to socket.
			free(ptr); // Free file memory.
		}
			close(fd); // Close the file.
		} // End if block for file found/not found.
	} // End if block for valid request.
} // End if block for valid HTTP.
shutdown(sockfd, SHUT_RDWR); // Close the socket gracefully.
}




int main(int argc, char const *argv[])
{
    int server_fd, new_socket; long valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int yes = 1;
    char *hello = "Hello from server\n";
    socklen_t sin_size;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In socket");
        exit(EXIT_FAILURE);
    }

    /* the setsockopt() function is simply used to set socket options. This function call sets the SO_REUSEADDR socket option
    to true which will allow to reuse a given address for binding.
    If a socket isn't closed properly, it may appear to be in use, so this option lets a socket bind to a port and take over
    control of it even if it seems to be in use 
    */
    
	if (setsockopt(server_fd,SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){ 

		perror("setting socket option SO_REUSEADDR");
		exit(EXIT_FAILURE);
	}


    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    
    memset(address.sin_zero, '\0', sizeof address.sin_zero);
    
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("In bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
    while(1)
    {
    sin_size = sizeof(struct sockaddr_in);
	new_socket = accept(server_fd, (struct sockaddr *)&address , &sin_size);
	if (new_socket == -1)
		perror("Accepting connection");
	
	handle_connection(new_socket, &address);
	}
    return 0;
}

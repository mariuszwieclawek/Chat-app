#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <linux/un.h>

#define MAXLINE 512
#define SA      struct sockaddr
#define IPV6 1
#define LISTENQ 10


int main(int argc, char **argv)
{
	int listenfd, connfd,n; // for server and client socket descriptor
	socklen_t len;
	const int on = 1;
	char *login; // for user login
	char mac[17]; // for user MAC Address 
	size_t bufsize = 50; // buffer size
	char buff[MAXLINE]; // buffer
	struct sockaddr_in	servaddr, cliaddr; // data for server and client
	char * readrow = NULL; // for read one row from file
	ssize_t readline; // for check if all lines is read from file 
	int check = 0; // for check if the user is regitered

	// socket on which we listen for connection
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr,"socket error : %s\n", strerror(errno));
        return 1;
    }

	// SO_REUSEADDR socket option allows a socket to bind to a port in use by another socket
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		return 1;
	}

	// clear and fill the server data structure
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr   = htonl(INADDR_ANY);
	servaddr.sin_port   = htons(13);

	// assigns the address to the socket descriptor
    if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        fprintf(stderr,"bind error : %s\n", strerror(errno));
        return 1;
    }

	// socket that will be used to accept incoming connection
    if ( listen(listenfd, LISTENQ) < 0){
        fprintf(stderr,"listen error : %s\n", strerror(errno));
        return 1;
    }

	fprintf(stderr,"Waiting for clients ... \n");

	for ( ; ; ) {
		len = sizeof(cliaddr);
			// accept the connection and create a socket
        	if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0){
                	fprintf(stderr,"accept error : %s\n", strerror(errno));
                	continue;
        	}

		bzero(buff,sizeof(buff));

	   	inet_ntop(AF_INET, (struct sockaddr  *) &cliaddr.sin_addr,  buff, sizeof(buff));

		printf("Connection from:\nIP: %s\n", buff);

		// get MAC Address
		read(connfd, buff, MAXLINE); 
		fprintf(stdout,"MAC: %s",buff);

		for(int i = 0 ; i < 17; i++){
			mac[i] = buff[i]; // copy
		}

		// reading from user data file
		FILE *users_data = fopen("users_data.txt","r");
		if (users_data == NULL) 
	            {   
	              printf("Error! Could not open file\n"); 
	            	return 1; 
	            }


		// reading every line in the file
		while((readline = getline(&readrow,&bufsize,users_data)) != -1){
			login = (char *)malloc(bufsize * sizeof(char));

			// reading the login from the file
			int mac_position;
			for(int i = 0; i< MAXLINE; i++){ 
				if(readrow[i] == ' '){
					mac_position = i;
					break;
				}
				login[i] = readrow[i];
			}


			// reading the MAC address from the file and checking if the program is running on the interface that has already been registered
			char mac_file[17];
			for(int i = mac_position+1, j = 0; i<MAXLINE; i++,j++){ // reading the saved MAC address from the file
				if(readrow[i] == '\n')
					break;
				mac_file[j] = readrow[i];
			}

			for(int k = 0; k<=sizeof(mac_file)-1; k++){ // checking if the MAC address already exists in the files (whether the user has already registered)
				if(mac_file[k] != mac[k]){ // when not exist
					free(login);
					check = 0;
					break;
				}
				else{	// when exist
					check = 1;
				}
			}

			if(check == 1) // don't check any further
				break;
		}

		fclose(users_data);
		if(readrow)
			free(readrow);

		// checking if the user exists and send information to client (send 1 to client)
		bzero(buff,sizeof(buff));
		if(check){
			fprintf(stdout,"%s connected!\n\n",login);
			snprintf(buff, sizeof(buff), "1%s",login);
	        	if( write(connfd, buff, strlen(buff))< 0 )
	                	fprintf(stderr,"write error : %s\n", strerror(errno));
		}
		else{ // when it does not exist, we register the user (send 0 to client)
			snprintf(buff, sizeof(buff), "0\n");
	        	if( write(connfd, buff, strlen(buff))< 0 )
	                	fprintf(stderr,"write error : %s\n", strerror(errno));
		}

		bzero(buff,sizeof(buff));
		// when user does not exist, we wait until he enters the login
		if(!check){
			int k;
			while ( (k = read(connfd, buff, MAXLINE)) < 3); // wait for login
			fprintf(stdout,"New user registered: %s\n\n",buff);

			// saving the user's data to a file when his data is not saved in the file (registration)
			users_data = fopen("users_data.txt","a");
			if (users_data == NULL){   
				printf("Error! Could not open file\n"); 
				return 1; 
			}

			fprintf(users_data, "%s ", buff);
			fprintf(users_data, "%s\n", mac);
			fclose(users_data);

		}
	}
}

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
	int sendfd, recvfd;
	const int on = 1;
	socklen_t salen;
	struct sockaddr	*sasend, *sarecv; // przechowuje inf o rodzinie adresu afinet
	struct sockaddr_in6 *ipv6addr; // to co nizej tylko dodatkowe opcje
	struct sockaddr_in *ipv4addr; //struktura przechowuje rodzine adresu,port,adres
	char *login; // zapiszemy tu login uzytkownika
	char mac[17]; // zapisujemy tu adres MAC uzytkownika
	size_t bufsize = 50; // wielkosc buforu
	char line[MAXLINE]; // bufor

	if (argc != 4){
		fprintf(stderr, "usage: %s  <IP-multicast-address> <port#> <if name>\n", argv[0]);
		return 1;
	}

	login = (char *)malloc(bufsize * sizeof(char));

/////////////////////////////////////////
	int listenfd, connfd,n;
	socklen_t len;
	char buff[MAXLINE], str[INET_ADDRSTRLEN+1];
	time_t	ticks;
	struct sockaddr_in	servaddr, cliaddr;

        if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                fprintf(stderr,"socket error : %s\n", strerror(errno));
                return 1;
        }

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		return 1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr   = htonl(INADDR_ANY);
	servaddr.sin_port   = htons(13);	/* daytime server */

        if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
                fprintf(stderr,"bind error : %s\n", strerror(errno));
                return 1;
        }

        if ( listen(listenfd, LISTENQ) < 0){
                fprintf(stderr,"listen error : %s\n", strerror(errno));
                return 1;
        }
	fprintf(stderr,"Waiting for clients ... \n");
	for ( ; ; ) {
		len = sizeof(cliaddr);
        	if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0){
                	fprintf(stderr,"accept error : %s\n", strerror(errno));
                	continue;
        	}

		bzero(str, sizeof(str));
		bzero(buff,sizeof(buff));
		bzero(line,sizeof(line));
	   	inet_ntop(AF_INET, (struct sockaddr  *) &cliaddr.sin_addr,  str, sizeof(str));
		printf("Connection from:\nIP: %s\n", str);

		read(connfd, buff, MAXLINE); // get address MAC
		fprintf(stdout,"MAC: %s",buff);

		for(int i = 0 ; i < 17; i++){
			mac[i] = buff[i];
		}

// odczyt z pliku danych uzytkownika
		FILE *users_data = fopen("users_data.txt","r");
		if (users_data == NULL) 
	            {   
	              printf("Error! Could not open file\n"); 
	            	return 1; 
	            } 
		fgets(line, MAXLINE, users_data); // odczyt z pliku do bufora calosci (login i mac)

		fclose(users_data);

// odczyt loginu z pliku
		int mac_position;
		for(int i = 0; i< MAXLINE; i++){ 
			if(line[i] == ' '){
				mac_position = i;
				break;
			}
			login[i] = line[i];
		}

/*		printf("login:");
		for(int i = 0; i< mac_position; i++){
			printf("%c",login[i]);
		}
		printf("\n");
*/

// odczyt adresu MAC z pliku i sprawdzenie czy uruchomiono program na interfejsie ktory juz sie rejestrowal
		char mac_file[17];
		int j=0;
		for(int i = mac_position+1; i<MAXLINE; i++,j++){ //odczyt z pliku zapisanego adresu MAC
			if(line[i] == '\n')
				break;
			mac_file[j] = line[i];
		}

		int check;
		for(int k = 0; k<=sizeof(mac_file)-1; k++){ //sprawdzenie czy adres mac juz istnieje w plikach (czy uzytkownik sie juz rejestrowal)
			if(mac_file[k] != mac[k]){ //kiedy nie istnieje
				check = 0;
				break;
			}
			else	//kiedy istnieje
				check = 1;
		}

//kontrola czy uzytkownik istnieje i wyswietlenie informacji dla uzytkownika
		bzero(buff,sizeof(buff));
		if(check){
			fprintf(stdout,"%s connected!\n\n",login);
			snprintf(buff, sizeof(buff), "1%s",login);
	        	if( write(connfd, buff, strlen(buff))< 0 )
	                	fprintf(stderr,"write error : %s\n", strerror(errno));
		}
		else{ //kiedy nie istnieje to rejestrujemy uzytkownika (podajemy login)
			snprintf(buff, sizeof(buff), "0\n");
	        	if( write(connfd, buff, strlen(buff))< 0 )
	                	fprintf(stderr,"write error : %s\n", strerror(errno));
		}

		bzero(buff,sizeof(buff));
		if(!check){
			int k;
			while ( (k = read(connfd, buff, MAXLINE)) < 3);
			fprintf(stdout,"New user registered: %s\n\n",buff);

// zapis do pliku danych uzytkownika kiedy jego dane nie sa zapisane w pliku (rejestracja)
			users_data = fopen("users_data.txt","w");
			if (users_data == NULL){   
				printf("Error! Could not open file\n"); 
				return 1; 
			}
			fprintf(users_data, "%s ", buff);
			fprintf(users_data, "%s\n", mac);
			fclose(users_data);
		}

	}
//////////////////////////////////////////




}

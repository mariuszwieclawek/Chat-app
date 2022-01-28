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


// the function gets the mac address from the interface
int get_mac_addr(char* name, char* mac){
	struct ifreq ifr;
	int sd,merr;
	sd = socket(PF_INET,SOCK_STREAM,0);
	if(sd < 0)
	{
		fprintf(stderr,"if_up: socket eroor %s\n", strerror(errno));
		return sd;
	}

	memset(&ifr,0,sizeof(ifr));
	sprintf(ifr.ifr_name,"%s",name);
	merr = ioctl(sd,SIOCGIFHWADDR,&ifr); //read MAC address
	if(merr < 0)
	{
		close(sd);
		return merr;
	}

	close(sd);

	char buf[19];
	sprintf(buf,"%02x:%02x:%02x:%02x:%02x:%02x\n",
		(int)((unsigned char *) &ifr.ifr_hwaddr.sa_data)[0],
		(int)((unsigned char *) &ifr.ifr_hwaddr.sa_data)[1],
		(int)((unsigned char *) &ifr.ifr_hwaddr.sa_data)[2],
		(int)((unsigned char *) &ifr.ifr_hwaddr.sa_data)[3],
		(int)((unsigned char *) &ifr.ifr_hwaddr.sa_data)[4],
		(int)((unsigned char *) &ifr.ifr_hwaddr.sa_data)[5]);
	
	strcpy(mac,buf);

}


// function to create a send socket for udp
int snd_udp_socket(const char *serv, int port, SA **saptr, socklen_t *lenp)
{
	int sockfd, n;
	struct addrinfo	hints, *res, *ressave;
	struct sockaddr_in6 *pservaddrv6;
	struct sockaddr_in *pservaddrv4;

	*saptr = malloc( sizeof(struct sockaddr_in6));

	pservaddrv6 = (struct sockaddr_in6*)*saptr;

	bzero(pservaddrv6, sizeof(struct sockaddr_in6));

	if (inet_pton(AF_INET6, serv, &pservaddrv6->sin6_addr) <= 0){

		free(*saptr);
		*saptr = malloc( sizeof(struct sockaddr_in));
		pservaddrv4 = (struct sockaddr_in*)*saptr;
		bzero(pservaddrv4, sizeof(struct sockaddr_in));

		if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) <= 0){
			fprintf(stderr,"AF_INET inet_pton error for %s : %s \n", serv, strerror(errno));
			return -1;
		}else{
			pservaddrv4->sin_family = AF_INET;
			pservaddrv4->sin_port   = htons(port);
			*lenp =  sizeof(struct sockaddr_in);
			if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
				fprintf(stderr,"AF_INET socket error : %s\n", strerror(errno));
				return -1;
			}
		}

	}else{
		pservaddrv6 = (struct sockaddr_in6*)*saptr;
		pservaddrv6->sin6_family = AF_INET6;
		pservaddrv6->sin6_port   = htons(port);	/* daytime server */
		*lenp =  sizeof(struct sockaddr_in6);

		if ( (sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
			fprintf(stderr,"AF_INET6 socket error : %s\n", strerror(errno));
			return -1;
		}

	}

	return(sockfd);
}
/* end send_udp_socket */


// the function returns which family of addresses we are using
int family_to_level(int family)
{
	switch (family) {
	case AF_INET:
		return IPPROTO_IP;
#ifdef	IPV6
	case AF_INET6:
		return IPPROTO_IPV6;
#endif
	default:
		return -1;
	}
}


// function joins the interface to a multicast group
int mcast_join(int sockfd, const SA *grp, socklen_t grplen,const char *ifname, u_int ifindex)
{
	struct group_req req;
	if (ifindex > 0) {
		req.gr_interface = ifindex;
	} else if (ifname != NULL) {
		if ( (req.gr_interface = if_nametoindex(ifname)) == 0) {
			errno = ENXIO;	/* if name not found */
			return(-1);
		}
	} else
		req.gr_interface = 0;
	if (grplen > sizeof(req.gr_group)) {
		errno = EINVAL;
		return -1;
	}
	memcpy(&req.gr_group, grp, grplen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),
			MCAST_JOIN_GROUP, &req, sizeof(req)));
}
/* end mcast_join */

int mcast_join_org(int sockfd, const SA *grp, socklen_t grplen,
		   const char *ifname, u_int ifindex)
{
#ifdef MCAST_JOIN_GROUP
	struct group_req req;
	if (ifindex > 0) {
		req.gr_interface = ifindex;
	} else if (ifname != NULL) {
		if ( (req.gr_interface = if_nametoindex(ifname)) == 0) {
			errno = ENXIO;	/* if name not found */
			return(-1);
		}
	} else
		req.gr_interface = 0;
	if (grplen > sizeof(req.gr_group)) {
		errno = EINVAL;
		return -1;
	}
	memcpy(&req.gr_group, grp, grplen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),
			MCAST_JOIN_GROUP, &req, sizeof(req)));
#else
/* end mcast_join1 */

/* include mcast_join2 */
	switch (grp->sa_family) {
	case AF_INET: {
		struct ip_mreq		mreq;
		struct ifreq		ifreq;

		memcpy(&mreq.imr_multiaddr,
			   &((const struct sockaddr_in *) grp)->sin_addr,
			   sizeof(struct in_addr));

		if (ifindex > 0) {
			if (if_indextoname(ifindex, ifreq.ifr_name) == NULL) {
				errno = ENXIO;	/* i/f index not found */
				return(-1);
			}
			goto doioctl;
		} else if (ifname != NULL) {
			strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
doioctl:
			if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0)
				return(-1);
			memcpy(&mreq.imr_interface,
				   &((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr,
				   sizeof(struct in_addr));
		} else
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);

		return(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
						  &mreq, sizeof(mreq)));
	}
/* end mcast_join2 */

/* include mcast_join3 */
#ifdef	IPV6
#ifndef	IPV6_JOIN_GROUP		/* APIv0 compatibility */
#define	IPV6_JOIN_GROUP		IPV6_ADD_MEMBERSHIP
#endif
	case AF_INET6: {
		struct ipv6_mreq	mreq6;

		memcpy(&mreq6.ipv6mr_multiaddr,
			   &((const struct sockaddr_in6 *) grp)->sin6_addr,
			   sizeof(struct in6_addr));

		if (ifindex > 0) {
			mreq6.ipv6mr_interface = ifindex;
		} else if (ifname != NULL) {
			if ( (mreq6.ipv6mr_interface = if_nametoindex(ifname)) == 0) {
				errno = ENXIO;	/* i/f name not found */
				return(-1);
			}
		} else
			mreq6.ipv6mr_interface = 0;

		return(setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
						  &mreq6, sizeof(mreq6)));
	}
#endif

	default:
		errno = EAFNOSUPPORT;
		return(-1);
	}
#endif
}
/* end mcast_join3_org */

int sockfd_to_family(int sockfd)
{
	struct sockaddr_storage ss;
	socklen_t	len;

	len = sizeof(ss);
	if (getsockname(sockfd, (SA *) &ss, &len) < 0)
		return(-1);
	return(ss.ss_family);
}

int mcast_set_loop(int sockfd, int onoff)
{
	switch (sockfd_to_family(sockfd)) {
	case AF_INET: {
		u_char		flag;

		flag = onoff;
		return(setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
						  &flag, sizeof(flag)));
	}

#ifdef	IPV6
	case AF_INET6: {
		u_int		flag;

		flag = onoff;
		return(setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
						  &flag, sizeof(flag)));
	}
#endif

	default:
		errno = EAFNOSUPPORT;
		return(-1);
	}
}
/* end mcast_set_loop */


void	recv_all(int, socklen_t);
void	send_all(int, SA *, socklen_t, char *);


void send_all(int sendfd, SA *sadest, socklen_t salen,char *login)
{
	char		line[MAXLINE];		/* hostname and process ID */
	char *text;
	struct utsname	myname;
	size_t bufsize = 1024;

	text = (char *)malloc(bufsize * sizeof(char));

	if (uname(&myname) < 0)
		perror("uname error");

	snprintf(line, sizeof(line), "%s connected to the chat room!\n", login);
	if(sendto(sendfd, line, strlen(line), 0, sadest, salen) < 0 )
		fprintf(stderr,"sendto() error : %s\n", strerror(errno));

	for ( ; ; ) {
		getline(&text,&bufsize,stdin);
		snprintf(line, sizeof(line), "%s: %s", login, text);
		if(sendto(sendfd, line, strlen(line), 0, sadest, salen) < 0 )
			fprintf(stderr,"sendto() error : %s\n", strerror(errno));
	}
}

void recv_all(int recvfd, socklen_t salen)
{
	int					n;
	char				line[MAXLINE+1];
	socklen_t			len;
	struct sockaddr		*safrom;
	char str[128];
	struct sockaddr_in6*	 cliaddr;
	struct sockaddr_in*	 cliaddrv4;
	char			addr_str[INET6_ADDRSTRLEN+1];

	safrom = malloc(salen);

	for ( ; ; ) {
		len = salen;
		if( (n = recvfrom(recvfd, line, MAXLINE, 0, safrom, &len)) < 0 )
		  perror("recvfrom() error");

		line[n] = 0;	/* null terminate */
		
		if( safrom->sa_family == AF_INET6 ){
		      cliaddr = (struct sockaddr_in6*) safrom;
		      inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr->sin6_addr,  addr_str, sizeof(addr_str));
		}
		else{
		      cliaddrv4 = (struct sockaddr_in*) safrom;
		      inet_ntop(AF_INET, (struct sockaddr  *) &cliaddrv4->sin_addr,  addr_str, sizeof(addr_str));
		}

		printf("%s", line);
		fflush(stdout);
	}
}



int main(int argc, char **argv)
{
	int servfd; // server socket TCP
	struct sockaddr_in	servaddr; // data structure for server
	int sendfd, recvfd; // sendind and receiving socket UDP
	const int on = 1;
	socklen_t salen;
	struct sockaddr	*sasend, *sarecv; // information about address family
	struct sockaddr_in6 *ipv6addr; 
	struct sockaddr_in *ipv4addr; 
	char *login; // for user login
	char* mac; // for user MAC Address
	size_t bufsize = 50; // buffer size
	char line[MAXLINE]; // buffer
	char send[MAXLINE + 1];
	char recvline[MAXLINE];
	int err; // for address ip filling error

	// check that all arguments are given
	if (argc != 5){
		fprintf(stderr, "usage: %s  <IP-multicast-address> <port#> <if name> <serv-address-IP>\n", argv[0]);
		return 1;
	}

	login = (char *)malloc(bufsize * sizeof(char));
	mac = (char *)malloc(bufsize * sizeof(char));

	// read MAC Address
	get_mac_addr(argv[3],mac);


	// create a socket for the server
	if ( (servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}

	// fill the server data structure
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(13);
	if ( (err=inet_pton(AF_INET, argv[4], &servaddr.sin_addr)) <= 0){
		if(err == 0 )
			fprintf(stderr,"inet_pton error for %s \n", argv[4] );
		else
			fprintf(stderr,"inet_pton error for %s : %s \n", argv[4], strerror(errno));
		return 1;
	}

	// connect to the server socket
	if (connect(servfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"connect error : %s \n", strerror(errno));
		return 1;
	}

	// send MAC Address to server
	bzero(send, sizeof(send));
	snprintf(send, sizeof(send), "%s",mac);
    if( write(servfd, send, sizeof(send))< 0 )
        fprintf(stderr,"write error : %s\n", strerror(errno));


	// wait for user identification
	//int n;
	bzero(recvline, sizeof(recvline));
	read(servfd,recvline,MAXLINE); // read from server
	char check_2 = recvline[0]; // we get 0 if unregistered or 1 if registered

	// check if the user is registered
	if(check_2 == '0'){ 
		printf("You are unregistered, please enter your nickname!\nLogin:");
		int no_read = getline(&login,&bufsize,stdin);
		login[no_read-1] = '\0'; 
		snprintf(send,sizeof(send),"%s",login);
	        if( write(servfd, send, sizeof(send))< 0 )
        	       	fprintf(stderr,"write error : %s\n", strerror(errno));
	}
	else{
		for(int i = 1; i < strlen(recvline); i++)
			login[i-1]=recvline[i];
		printf("Welcome back %s!\nYou can start texting:\n",login);
	}


	// the function on the basis of 4/6 address and port creates a sending socket and fills the sasend address structure
	sendfd = snd_udp_socket(argv[1], atoi(argv[2]), &sasend, &salen);

	// create the receiving socket, the address family from the sasend structure
	if ( (recvfd = socket(sasend->sa_family, SOCK_DGRAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}

	// SO_REUSEADDR, in one station we can run several processes and they will listen on the same port and will be able to receive data between them
	if (setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		return 1;
	}

	sarecv = malloc(salen);
	memcpy(sarecv, sasend, salen);

	// binding socket to the interface
	setsockopt(sendfd, SOL_SOCKET, SO_BINDTODEVICE, argv[3], strlen(argv[3]));


	// assigns the address to the socket descriptor
	if( bind(recvfd, sarecv, salen) < 0 ){
	    fprintf(stderr,"bind error : %s\n", strerror(errno));
	    return 1;
	}

	// connect to the multicast group
	if( mcast_join(recvfd, sasend, salen, argv[3], 0) < 0 ){
		fprintf(stderr,"mcast_join() error : %s\n", strerror(errno));
		return 1;
	}

	//
	mcast_set_loop(sendfd, 1);

	if (fork() == 0)
		recv_all(recvfd, salen);	/* child -> receives */

	send_all(sendfd, sasend, salen, login);	/* parent -> sends */
}

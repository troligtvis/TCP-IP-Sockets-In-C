//
// client2.c
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "3490"	// the port client will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once
#define commandoMAX 100

// get sockaddrs, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}	

	return &(((struct sockaddr_in6*)sa)->sin6_addr);

}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv, t;
	char s[INET6_ADDRSTRLEN];
	char str[100];

	if(argc != 2)
	{
		fprintf(stderr, "usage: client hostname\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}

		if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{ 
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if(p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

 
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	// egen kod

  while(printf("Ange kommando > "), fgets(str, 1000, stdin), !feof(stdin)) 
  { 
 
    int i;
    for (i = 0; i < strlen(str); i++)
    {
      if(str[i] == '\n')
      {
        str[i] = '\0'; 
      }
    }

    //skickar string till server
    if (send(sockfd, str, commandoMAX, 0) == -1) 
    {
      perror("send");
      exit(1);
    }
    
    for(;;)
    {
      if((t=recv(sockfd, str, commandoMAX, 0)) > 0)
      {
        str[t] = '\0';
      }
      else 
      {
        if (t < 0) perror("recv");
        else 
        {
          break;
        }
      }

      if(strcmp(str, "ENDOFFILE\n") == 0)
        break;
      else
        printf("%s", str);
      }//for-loopen
  }//while-loopen
close(sockfd);

	return 0;
}


































//
//server2.c
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#define PORT "3490" // the port users will be connecting to
#define BACKLOG 10 // how many pending connections qeue will hold
#define commandoMAX 100
#define READ 0
#define WRITE 1

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static void daemonize()
{
    pid_t pid, sid;

    /* already a daemon */
    if ( getppid() == 1 ) return;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/tmp")) < 0) {
        exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
}

//sparar demonens PID i en fil
void sparaDemonPID(int *dpid)
{
  FILE *fp;

  fp = fopen("/home/me/demonPID.txt", "w+");
  fwrite(dpid, 4, 1, fp);
  fclose(fp);
}

int main(void)
{
	int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;	//connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	pid_t pid;
	char destinationen[100], str[1000];

 	daemonize();
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;	//use my ip

	if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if((sockfd = socket(p->ai_family,p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("server: socket");
			continue;
		}

		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}

		if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if(p == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if(listen(sockfd, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}

	int id = getpid();
 	sparaDemonPID(&id);

	printf("server: waiting for connections...\n");

	while(1) //main accept() loop
	{
		int done, n;
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if(new_fd == -1)
		{
			perror("accept");
			continue;
		}
			inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
			printf("server: got connection from %s\n", s);

			//egen kod
    switch(fork())
    {
      case 0:
        close(WRITE);
        dup(new_fd);
        //close(sockfd); //test 
        done = 0;
  
        do{
          n = recv(new_fd, str, commandoMAX, 0); // tar emot string från client 
          if (n <= 0) 
          {
            if (n < 0) perror("recv");
            done = 1;
            exit(1);
          }
          if(!done)  
          {  
            //kontrollerar ifall stängen innehåller cd på plats 0 och 1.
            if(str[0]=='c' && str[1]=='d')
            {
              strcpy(destinationen, &str[3]);

              if(chdir(destinationen) < 0)
                {
                   perror("couldn't change directory");
                }
              //printf("After cd: %s\n", getcwd(destinationen, sizeof(destinationen)));
            } 
            else
            {
              //skapar ett barn för att utföra ett bashkommando för att inte
              //avsluta hela servern. 
              switch(fork())
              {
                case 0:
                  close(WRITE);
                  dup(new_fd);
                  execlp("/bin/sh", "sh", "-c", str, NULL);
                break;
                }   
              wait(0); //föräldern väntar på att barnet ska bli klar
            }
            //avslutar sändningen av output med hjälp av echo, som skriver
            //ENDOFFILE! för att låta clienten skriva nytt kommando.
            switch(fork())
            {
              case -1:
                perror("fork perror");
                exit(1);
                break;

              case 0:
               close(WRITE);
               dup(new_fd);
               execlp("/bin/echo", "echo", "ENDOFFILE", NULL);
               exit(1);
              break;
             }
           wait(0); //föräldern väntar på att barnet ska bli klar
           //close(new_fd);
          }    
        } while (!done);

      if(pid == 0)
      {
        exit(1);
      }
      close(new_fd);
    } 

	close(new_fd);

	}
	return 0;
}

























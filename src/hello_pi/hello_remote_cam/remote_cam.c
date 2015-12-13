/*
Remote Camera 

Must accept commands from the main Pi

Commands should include
   Take Photo
   Set Preview Resolution
   Start Preview
   Stop Preview

Desirable commands
   White Balence
   Other camera Options

Preview is sent via TCP protocal so that the relevent buffer and information can be sent 1 after the other
(only othe option is to write my own protocol using UDP prepacking the buffer with relevent info)
Having looked at the options TCP offers the best option
NOTE this will not be portable code (due to it realying both ends to have the same endieness) !

Even while displaying preview remote camera should accept and process commands
The commands will be identified by an integer followed by the relevent(if any) information to be processed
when no command is needed the main pi will send 0 commands will use the same TCP connection
 */




#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define IP_ADD "127.0.0.1"
#define SERV_PORT "8033"

#define SOCKTYPE_TCP 1
#define SOCKTYPE_UDP 2

//fuction prototypes
int getAndConnectSocket(int socket_type);


// MAIN

int main(int argc, char *argv[])
{
  int numbytes;
  int socket_fd;

  socket_fd = getAndConnectSocket(SOCKTYPE_UDP);
  printf("socket_fd = %d", socket_fd);

  

  return 0;
}


//fuctions
//returns a bound socket
int getAndConnectSocket(int socket_type)
{
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int return_value = 0;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  if(socket_type == SOCKTYPE_UDP)
    hints.ai_socktype = SOCK_DGRAM;
  if(socket_type == SOCKTYPE_TCP)
    hints.ai_socktype = SOCK_STREAM;
  else
    {
      printf("oops\n");
      return -2;
    }

  //use getaddrinfo to populate servinfo
  return_value = getaddrinfo(IP_ADD, SERV_PORT, &hints, &servinfo);
  if (return_value != 0)
    {
      fprintf(stderr, "Error using getaddrinfo, Error: %s\n", gai_strerror(return_value));
      exit(EXIT_FAILURE);
    }

  //loop through results to get a valid socket
  for(p = servinfo; p != NULL; p = p->ai_next)
    {
      //attempt get socket
      sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if(sockfd == -1)
	{
	  fprintf(stderr, "Error using socket function\n");
	  continue;
	}
      //attempt connect on socket! (will bind socket to free port automatically)
      return_value = connect(sockfd, p->ai_addr, p->ai_addrlen);
      if(return_value == -1)
	{
	  fprintf(stderr, "Error connecting to socket\n");
	  continue;
	}
      
      break;
    }

  if (p == NULL)
    {
      fprintf(stderr, "talker:failed to connect on socket\n");
      return -1;
    }

  freeaddrinfo(servinfo);
  return sockfd;
}

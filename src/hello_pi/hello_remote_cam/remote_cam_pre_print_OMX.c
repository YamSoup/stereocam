/*
Remote Camera

Must accept commands from the main Pi

Commands should include
   Take Photo
   Request Preview Resolution (include in start preview)
   Start Preview
   Stop Preview

Desirable commands
   White Balence
   Other camera Options

NOTE this will not be portable code (due to it realying both ends to have the same endieness) !

Even while displaying preview remote camera should accept and process commands
The commands will be identified by an integer followed by the relevent(if any) information to be processed
when no command is needed the main pi will send NO_COMMAND the commands will use the same TCP connection
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "bcm_host.h"
#include "ilclient.h"
#include "print_OMX.h"

//#define IP_ADD "127.0.0.1"
#define IP_ADD "192.168.0.13"
#define SERV_PORT "8039"

//possibly put this enum in a header file to easily include in other programs
enum rcam_command
{
    NO_COMMAND = 0,
    SET_PREVIEW_RES = 10,
    SET_PREVIEW_FRAMERATE = 11,
    START_PREVIEW = 20,
    STOP_PREVIEW = 21,
    TAKE_PHOTO = 30
};

////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
////////////////////////////////////////////////////////////////


//move these into seperate libary for sockets
#define SOCKTYPE_TCP 1
#define SOCKTYPE_UDP 2
void write_all(int socket, const void *buf, size_t num_bytes);
void read_all(int socket, const void *buf, size_t num_bytes);
int getAndConnectSocket(int socket_type);

//move these into a seperate libary for printing OMX things
//void print_OMX_PARAM_PORTDEFINITIONTYPE(OMX_PARAM_PORTDEFINITIONTYPE params);
//void printBits(void *toPrint);
//void printState(OMX_HANDLETYPE handle);
//char *err2str(int err);

void setCaptureRes(COMPONENT_T *camera, int width, int height);
void setPreviewRes(COMPONENT_T *camera, int width, int height);

void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data);


// MAIN
int main(int argc, char *argv[])
{
    int preview_width, preview_height, count = 0;
    int socket_fd;
    enum rcam_command current_command;
    bool deliver_preview = false;

    ILCLIENT_T *client;
    COMPONENT_T *camera;
    OMX_ERRORTYPE OMXstatus;

    OMX_BUFFERHEADERTYPE *previewHeader;

    //INITIALIZE CAMERA STUFF

    //initialize bcm host
    bcm_host_init();

    //create client
    client = ilclient_init();
    if(client == NULL)
    {
        fprintf(stderr, "unable to initialize ilclient\n");
        exit(EXIT_FAILURE);
    }

    //initialize OMX
    OMXstatus = OMX_Init();
    if (OMXstatus != OMX_ErrorNone)
    {
        fprintf(stderr, "unable to initialize OMX");
        ilclient_destroy(client);
        exit(EXIT_FAILURE);
    }

    // set error callback
    ilclient_set_error_callback(client,
                                error_callback,
                                NULL);

    //initialize camera
    ilclient_create_component(client,
                              &camera,
                              "camera",
                              ILCLIENT_DISABLE_ALL_PORTS
			      | ILCLIENT_ENABLE_OUTPUT_BUFFERS);
    printState(ilclient_get_handle(camera));

    OMXstatus = ilclient_change_component_state(camera, OMX_StateIdle);
    if (OMXstatus != OMX_ErrorNone)
    {
        fprintf(stderr, "unable to move camera component to Idle (1)");
        exit(EXIT_FAILURE);
    }
    printState(ilclient_get_handle(camera));

    //set the capture resolution
    //setCaptureRes(camera, 2592, 1944);
    //set default preview resolution
    setPreviewRes(camera, 320, 240);

    //assign the buffers
    ilclient_enable_port_buffers(camera, 70, NULL, NULL, NULL);
    ilclient_enable_port(camera, 70);
    printState(ilclient_get_handle(camera));

    //change the camera state to executing
    OMXstatus = ilclient_change_component_state(camera, OMX_StateExecuting);
    if (OMXstatus != OMX_ErrorNone)
    {
        fprintf(stderr, "unable to move camera component to Executing (1)\n");
        exit(EXIT_FAILURE);
    }
    printState(ilclient_get_handle(camera));

    //SOCKET STUFF
    socket_fd = getAndConnectSocket(SOCKTYPE_TCP);
    if (socket_fd < 0)
      {
	printf("socket failure\n");
	exit(EXIT_FAILURE);
      }
    else
      printf("socket success!, socket_fd = %d\n", socket_fd);


    ////////////////////////////////////////////////////////////
    // SEND AND RECV
    ////////////////////////////////////////////////////////////

    char handshake_r[4];
    long int num_bytes = 0;
    //handshake
    printf("sending handshake\n");
    write(socket_fd, "handshake\0", sizeof(char) * 11);
    printf("handshake sent\n");
    read(socket_fd, &handshake_r, sizeof(char)*4);
    printf("handshake = %s\n\n", handshake_r);

    while(count < 100)
    {
      count++;
      printf("count = %d\n", count);

      //get command
      printf("waiting for command\n");
      read(socket_fd, &current_command, sizeof(current_command));
      printf("got command = %d\n", (int)current_command);      

      switch(current_command)
	{
	case NO_COMMAND: break;
	case START_PREVIEW: deliver_preview = true; break;
	case STOP_PREVIEW: deliver_preview = false; break;
	}

      //if preview is running deliver preview
      if (deliver_preview == true)
	{
	  //print state (to check its still executing
	  printState(ilclient_get_handle(camera));
	  
	  //get buffer from camera
	  OMXstatus = OMX_FillThisBuffer(ilclient_get_handle(camera), previewHeader);
	  previewHeader = ilclient_get_output_buffer(camera, 70, 1);

	  //send buffer, checks lengths to ensure all data is sent
	  printf("nAllocLen = %d\n", previewHeader->nAllocLen);
	  printf("sending buffer ... ");
	  num_bytes = write(socket_fd, previewHeader->pBuffer, previewHeader->nAllocLen);
	  while (num_bytes < previewHeader->nAllocLen)
	    num_bytes += write(socket_fd, previewHeader->pBuffer + num_bytes, previewHeader->nAllocLen - num_bytes);	  
	  printf("buffer sent, %ld bytes \n", num_bytes);
	}
      else
	{
	  ;//do nothing;
	}
    }//end of while loop


    //make camera idle again
    OMXstatus = ilclient_change_component_state(camera, OMX_StateIdle);
    if (OMXstatus != OMX_ErrorNone)
    {
        fprintf(stderr, "unable to move camera component to Idle (1)");
        exit(EXIT_FAILURE);
    }
    printState(ilclient_get_handle(camera));

    return 0;
}


/////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////

//write all (to ensure full buffer is sent)

void write_all(int socket, const void *buf, size_t num_bytes)
{
  size_t current_writen = 0;
  while (current_writen < num_bytes)
    current_writen += write(socket, &buf + current_writen, num_bytes - current_writen);
}

void read_all(int socket, const void *buf, size_t num_bytes)
{
  size_t current_read = 0;
  while (current_read < num_bytes)
    current_read += read(socket, &buf + current_read, num_bytes - current_read);
}

//set capture res
void setCaptureRes(COMPONENT_T *camera, int width, int height)
{
  OMX_PARAM_PORTDEFINITIONTYPE port_params;
  OMX_ERRORTYPE OMXstatus;

  memset(&port_params, 0, sizeof(port_params));
  port_params.nVersion.nVersion = OMX_VERSION;
  port_params.nSize = sizeof(port_params);
  port_params.nPortIndex = 72;

  OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter In setCaptureRes. Error = %s\n", err2str(OMXstatus));
  //change needed params
  port_params.format.image.nFrameWidth = width; //maxsettings
  port_params.format.image.nFrameHeight = height;
  port_params.format.image.nStride = 0; //needed! set to 0 to recalculate
  port_params.format.image.nSliceHeight = 0;  //notneeded?
  //set changes
  OMXstatus = OMX_SetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter In setCaptureRes. Error = %s\n", err2str(OMXstatus));

}

//set preview res
void setPreviewRes(COMPONENT_T *camera, int width, int height)
{
    //needs to check width and height to see if compatible with rpi

    OMX_PARAM_PORTDEFINITIONTYPE port_params;
    OMX_ERRORTYPE OMXstatus;

    memset(&port_params, 0, sizeof(port_params));
    port_params.nVersion.nVersion = OMX_VERSION;
    port_params.nSize = sizeof(port_params);
    port_params.nPortIndex = 70;
    //prepopulate structure
    OMXstatus = OMX_GetConfig(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
    if (OMXstatus != OMX_ErrorNone)
        printf("Error Getting Parameter In setPreviewRes. Error = %s\n", err2str(OMXstatus));
    //change needed params
    port_params.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
    port_params.format.video.nFrameWidth = width;
    port_params.format.video.nFrameHeight = height;
    port_params.format.video.nStride = width;
    port_params.format.video.nSliceHeight = height;
    port_params.format.video.xFramerate = 24 << 16;
    //set changes
    OMXstatus = OMX_SetConfig(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
    if (OMXstatus != OMX_ErrorNone)
        printf("Error Setting Parameter In setPreviewRes. Error = %s\n", err2str(OMXstatus));
    
    //print current config
    OMXstatus = OMX_GetConfig(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
    if (OMXstatus != OMX_ErrorNone)
        printf("Error Getting Parameter (2) In setPreviewRes. Error = %s\n", err2str(OMXstatus));
    print_OMX_PARAM_PORTDEFINITIONTYPE(port_params);


}





//returns a bound socket
int getAndConnectSocket(int socket_type)
{
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int return_value = 0;

  printf("%d", socket_type);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  if(socket_type == SOCKTYPE_UDP)
    hints.ai_socktype = SOCK_DGRAM;
  else if(socket_type == SOCKTYPE_TCP)
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

//converts OMX error to readable string
char *err2str(int err) {
  switch (err) {
  case OMX_ErrorInsufficientResources: return "OMX_ErrorInsufficientResources";
  case OMX_ErrorUndefined: return "OMX_ErrorUndefined";
  case OMX_ErrorInvalidComponentName: return "OMX_ErrorInvalidComponentName";
  case OMX_ErrorComponentNotFound: return "OMX_ErrorComponentNotFound";
  case OMX_ErrorInvalidComponent: return "OMX_ErrorInvalidComponent";
  case OMX_ErrorBadParameter: return "OMX_ErrorBadParameter";
  case OMX_ErrorNotImplemented: return "OMX_ErrorNotImplemented";
  case OMX_ErrorUnderflow: return "OMX_ErrorUnderflow";
  case OMX_ErrorOverflow: return "OMX_ErrorOverflow";
  case OMX_ErrorHardware: return "OMX_ErrorHardware";
  case OMX_ErrorInvalidState: return "OMX_ErrorInvalidState";
  case OMX_ErrorStreamCorrupt: return "OMX_ErrorStreamCorrupt";
  case OMX_ErrorPortsNotCompatible: return "OMX_ErrorPortsNotCompatible";
  case OMX_ErrorResourcesLost: return "OMX_ErrorResourcesLost";
  case OMX_ErrorNoMore: return "OMX_ErrorNoMore";
  case OMX_ErrorVersionMismatch: return "OMX_ErrorVersionMismatch";
  case OMX_ErrorNotReady: return "OMX_ErrorNotReady";
  case OMX_ErrorTimeout: return "OMX_ErrorTimeout";
  case OMX_ErrorSameState: return "OMX_ErrorSameState";
  case OMX_ErrorResourcesPreempted: return "OMX_ErrorResourcesPreempted";
  case OMX_ErrorPortUnresponsiveDuringAllocation: return "OMX_ErrorPortUnresponsiveDuringAllocation";
  case OMX_ErrorPortUnresponsiveDuringDeallocation: return "OMX_ErrorPortUnresponsiveDuringDeallocation";
  case OMX_ErrorPortUnresponsiveDuringStop: return "OMX_ErrorPortUnresponsiveDuringStop";
  case OMX_ErrorIncorrectStateTransition: return "OMX_ErrorIncorrectStateTransition";
  case OMX_ErrorIncorrectStateOperation: return "OMX_ErrorIncorrectStateOperation";
  case OMX_ErrorUnsupportedSetting: return "OMX_ErrorUnsupportedSetting";
  case OMX_ErrorUnsupportedIndex: return "OMX_ErrorUnsupportedIndex";
  case OMX_ErrorBadPortIndex: return "OMX_ErrorBadPortIndex";
  case OMX_ErrorPortUnpopulated: return "OMX_ErrorPortUnpopulated";
  case OMX_ErrorComponentSuspended: return "OMX_ErrorComponentSuspended";
  case OMX_ErrorDynamicResourcesUnavailable: return "OMX_ErrorDynamicResourcesUnavailable";
  case OMX_ErrorMbErrorsInFrame: return "OMX_ErrorMbErrorsInFrame";
  case OMX_ErrorFormatNotDetected: return "OMX_ErrorFormatNotDetected";
  case OMX_ErrorContentPipeOpenFailed: return "OMX_ErrorContentPipeOpenFailed";
  case OMX_ErrorContentPipeCreationFailed: return "OMX_ErrorContentPipeCreationFailed";
  case OMX_ErrorSeperateTablesUsed: return "OMX_ErrorSeperateTablesUsed";
  case OMX_ErrorTunnelingUnsupported: return "OMX_ErrorTunnelingUnsupported";
  default: return "unknown error";
  }
}

//error callback for OMX
void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data)
{
  fprintf(stderr, "OMX error %s\n", err2str(data));
}

//Prints the 0's and 1's of a char (8 bit byte)

void printBits(void *toPrint)
{
  unsigned char *b = (unsigned char*) toPrint;
  unsigned char byte;
  int i, j;
  size_t size = sizeof(*toPrint);

  for (i = size -1; i >= 0; i--)
    {
      for (j = 7; j >= 0; j--)
	{
	  byte = b[i] & (1<<j);
	  byte >>= j;
	  printf("%u", byte);
	}
    }
}

//Prints the state of a component

void printState(OMX_HANDLETYPE handle) {
  OMX_STATETYPE state;
  OMX_ERRORTYPE err;

  err = OMX_GetState(handle, &state);
  if (err != OMX_ErrorNone) {
    fprintf(stderr, "Error on getting state\n");
    exit(1);
  }
  switch (state) {
  case OMX_StateLoaded:           printf("StateLoaded\n"); break;
  case OMX_StateIdle:             printf("StateIdle\n"); break;
  case OMX_StateExecuting:        printf("StateExecuting\n"); break;
  case OMX_StatePause:            printf("StatePause\n"); break;
  case OMX_StateWaitForResources: printf("StateWait\n"); break;
  case OMX_StateInvalid:          printf("StateInvalid\n"); break;
  default:                        printf("State unknown\n"); break;
  }
}



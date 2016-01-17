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

#define IP_ADD "127.0.0.1"
#define SERV_PORT "8033"

#define SOCKTYPE_TCP 1
#define SOCKTYPE_UDP 2

////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
////////////////////////////////////////////////////////////////

int getAndConnectSocket(int socket_type);

void setCaptureRes(COMPONENT_T *camera, int width, int height);

void setPreviewRes(COMPONENT_T *camera, int width, int height);

void printBits(void *toPrint);

void printState(OMX_HANDLETYPE handle);

char *err2str(int err);

void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data);

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

// MAIN

int main(int argc, char *argv[])
{
    int numbytes, preview_width, preview_height;
    int socket_fd;
    enum rcam_command current_command;
    bool deliver_preview = false;

    ILCLIENT_T *client;
    COMPONENT_T *camera;
    OMX_ERRORTYPE OMXstatus;

    OMX_BUFFERHEADERTYPE **previewHeader;

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
                              ILCLIENT_DISABLE_ALL_PORTS);
    OMXstatus = ilclient_change_component_state(camera, OMX_StateIdle);
    if (OMXstatus != OMX_ErrorNone)
    {
        fprintf(stderr, "unable to move camera component to Idle (1)");
        exit(EXIT_FAILURE);
    }

    ilclient_enable_port_buffers(camera, 70, NULL, NULL, NULL);

    //change the camera state to executing
    OMXstatus = ilclient_change_component_state(camera, OMX_StateExecuting);
    if (OMXstatus != OMX_ErrorNone)
    {
        fprintf(stderr, "unable to move camera component to Executing (1)\n");
        exit(EXIT_FAILURE);
    }
    printState(ilclient_get_handle(camera));


    //set the capture resolution
    setCaptureRes(camera, 2592, 1944);

    //set default preview resolution
    setPreviewRes(camera, 320, 240);


    //SOCKET STUFF
    socket_fd = getAndConnectSocket(SOCKTYPE_TCP);
    printf("socket_fd = %d", socket_fd);

    //recv command
    recv(socket_fd, &current_command, sizeof(current_command), 0);

    //handshake?

    //process command
    if(current_command == NO_COMMAND)
    {
        //do nothing
    }
    else if (current_command == SET_PREVIEW_RES)
    {
        recv(socket_fd, &preview_height, sizeof(int), 0);
        recv(socket_fd, &preview_width, sizeof(int), 0);
        //check resolution is sane and alter if not (or do in setPreviewRes function)
        //use resv information to change the preview res
        setPreviewRes(camera, preview_height, preview_width);
    }
    else if (current_command == START_PREVIEW)
    {
        //check if preview is already running
        if (deliver_preview == true)
        {
            fprintf(stderr, "Preview is already running");
            //possibly send error back to other app
        }
        else
        {
            //change deliver_preview to true
            deliver_preview = true;
        }
    }
    else if (current_command == STOP_PREVIEW)
    {
        //stop preview
        //change deliver_preview to false
        deliver_preview == false;

    }
    else if (current_command == TAKE_PHOTO)
    {
        //take photo
    }

    //if preview is running deliver preview
    if (deliver_preview == true)
    {
        memset(&previewHeader, 0, sizeof(previewHeader));
        previewHeader.nVersion.nVersion = OMX_VERSION;
        previewHeader.nSize = sizeof(previewHeader);
        previewHeader.nOutputPortIndex = 70;
        //deliver preview
        OMX_FillThisBuffer(camera, &previewHeader);

        //needs to check lengths to ensure all data is sent
        //send(socket_fd, BUFFER_HEADER, sizeof(), 0)
        //send(socker_fd, BUFFER, sizeof(), 0
    }
    else
    {
        //do nothing
    }

    return 0;
}


/////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////

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
    OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
    if (OMXstatus != OMX_ErrorNone)
        printf("Error Getting Parameter In setPreviewRes. Error = %s\n", err2str(OMXstatus));
    //change needed params
    port_params.format.video.nFrameWidth = width;
    port_params.format.video.nFrameHeight = height;
    port_params.format.video.nStride = 0;
    port_params.format.video.nSliceHeight = 0;
    port_params.format.video.nBitrate = 0;
    port_params.format.video.xFramerate = 0;
    //set changes
    OMXstatus = OMX_SetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
    if (OMXstatus != OMX_ErrorNone)
        printf("Error Setting Parameter In setPreviewRes. Error = %s\n", err2str(OMXstatus));
}


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

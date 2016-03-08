  /*
    use ilclient_GetParameter, ilclient_SetParameter to setup components before executing state
    use ilclient_GetConfig and ilclient_SetConfig to change settings while in executing state
    
    some settings can be changed before executing and some while executing

    all 4 functions use OMX_INDEXTYPE enumeration to specify what settings are being changed

    for index for each component look at
    http://www.jvcref.com/files/PI/documentation/ilcomponents/index.html

    for relevent data stuctures see
    http://maemo.org/api_refs/5.0/beta/libomxil-bellagio/_o_m_x___index_8h.html 

  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bcm_host.h"
#include "ilclient.h"


//includes needed for sockets
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//defines
#define PORT "8033"

/////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
/////////////////////////////////////////////////////////////////


void printBits(void *toPrint);

void printState(OMX_HANDLETYPE handle);

void print_OMX_CONFIG_DISPLAYREGIONTYPE(OMX_CONFIG_DISPLAYREGIONTYPE current);

char *err2str(int err);

void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data); 


/////////////////////////////////////////////////////////////////
// MAIN
/////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  /////////////////////////////////////////////////////////////////
  // VARIABLES
  /////////////////////////////////////////////////////////////////

  ILCLIENT_T *client;

  COMPONENT_T *video_render = NULL;
  OMX_ERRORTYPE OMXstatus;
  uint32_t screen_width = 0, screen_height = 0;
  OMX_BUFFERHEADERTYPE grabbed;

  char char_buffer[11];
  int numbytes;

  OMX_CONFIG_DISPLAYREGIONTYPE render_config;
  memset(&render_config, 0, sizeof(render_config));
  render_config.nVersion.nVersion = OMX_VERSION;
  render_config.nSize = sizeof(render_config);
  render_config.nPortIndex = 90;

  /////////////////////////////////////////////////////////////////
  // SOCKET STUFF
  /////////////////////////////////////////////////////////////////

  //stuctures and vars and stuff
  int socket_fd;
  struct addrinfo hints, *results;
  struct sockaddr_storage remote_cam_addr;
  socklen_t addr_size = sizeof(remote_cam_addr);
  int socket_status = 0, recv_size; 
  

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;
  
  // getaddrinfo 
  socket_status = getaddrinfo(NULL, "8033", &hints, &results);
  if(socket_status != 0)
    {
      fprintf(stderr, "getaddrinfo failed, error = %s\n", gai_strerror(socket_status));
      exit(EXIT_FAILURE);
    }

  // socket
  socket_fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
  if(socket_fd <= 0)
    {
      fprintf(stderr, "socket failed\n");
      exit(EXIT_FAILURE);
    }

  // bind
  socket_status = bind(socket_fd, results->ai_addr, results->ai_addrlen);
  if(socket_status != 0)
    {
      fprintf(stderr, "bind failed\n");
      exit(EXIT_FAILURE);
    }

  freeaddrinfo(results);

  printf("socket = %d\n", socket_fd);

  numbytes = recv(socket_fd, &char_buffer, 11, 0);
  if (numbytes != 11)
    fprintf(stderr, "did not recv whole msg");
  else
    printf("%s", char_buffer);


  /////////////////////////////////////////////////////////////////
  // FORK
  /////////////////////////////////////////////////////////////////
#ifdef FORK

  pid_t pid;

  pid = fork();
  if (pid == 0)
    printf("in child process!!!\n");  //call a fuction etc
  if (pid < 0)
    fprintf(stderr, "Fork failed!\n");
  else /* if pid > 0 */
    printf("in main process\n");
  
#endif

  /////////////////////////////////////////////////////////////////
  // STARTUP
  /////////////////////////////////////////////////////////////////

  //initialise bcm_host
  bcm_host_init();

  //create client
  client = ilclient_init();
  if(client == NULL)
    {
      fprintf(stderr, "unable to initalize ilclient\n");
      exit(EXIT_FAILURE);
    }

  //initalize OMX
  OMXstatus = OMX_Init();
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to initalize OMX");
      ilclient_destroy(client);
      exit(EXIT_FAILURE);
    }

  //set error callback
  ilclient_set_error_callback(client,
			      error_callback,
			      NULL);

  //super special broadcom only function
  graphics_get_display_size(0/*framebuffer 0*/, &screen_width, &screen_height);
  printf("screen_width = %d, screen_height = %d\n", (int)screen_width, (int)screen_height);
  
  
  /////////////////////////////////////////////////////////////////
  // Initalize Components
  /////////////////////////////////////////////////////////////////
  
  ///////////////////////////////////////////
  ////Initialise video render////
  ///////////////////////////////////////////
  ilclient_create_component(client,
			    &video_render,
			    "video_render",
			    ILCLIENT_DISABLE_ALL_PORTS
			    );

  OMXstatus = ilclient_change_component_state(video_render, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move render component to Idle (1)\n");
      exit(EXIT_FAILURE);
    }
  
  render_config.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT
					   |OMX_DISPLAY_SET_FULLSCREEN
					   |OMX_DISPLAY_SET_NOASPECT
					   |OMX_DISPLAY_SET_MODE);
  render_config.fullscreen = OMX_FALSE;
  render_config.noaspect = OMX_FALSE;
 
  render_config.dest_rect.width = screen_width/2;
  render_config.dest_rect.height = screen_height;

  render_config.mode = OMX_DISPLAY_MODE_LETTERBOX;
  
  OMXstatus = OMX_SetConfig(ilclient_get_handle(video_render), OMX_IndexConfigDisplayRegion, &render_config);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter. Error = %s\n", err2str(OMXstatus));  
  
  /*
  DOES NOT WORK LEAVING AS A REMINDER

  memset(&render_config, 0, sizeof(render_config));
  render_config.nVersion.nVersion = OMX_VERSION;
  render_config.nSize = sizeof(render_config);
  render_config.nPortIndex = 90; 
  render_config.set = OMX_DISPLAY_SET_DUMMY;
  
  OMXstatus = OMX_GetConfig(ilclient_get_handle(video_render), OMX_IndexConfigDisplayRegion, &render_config);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Getting Config. Error = %s\n", err2str(OMXstatus));  
  print_OMX_CONFIG_DISPLAYREGIONTYPE(render_config);
  */


  /////////////////////////////////////////////////////////////////
  // Main Meat
  /////////////////////////////////////////////////////////////////

  //change preview render to executing
  OMXstatus = ilclient_change_component_state(video_render, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move video render component to Executing (1)\n");
      exit(EXIT_FAILURE);
    }
  printState(ilclient_get_handle(video_render));
  
  //sleep for 2 secs
  sleep(2);
  

  /////////////////////////////////////////////////////////////////
  //CLEANUP
  /////////////////////////////////////////////////////////////////

  //Disable components
  

  //check all components have been cleaned up
  OMX_Deinit();

  //destroy client
  ilclient_destroy(client);

  return 0;
}


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

void print_OMX_CONFIG_DISPLAYREGIONTYPE(OMX_CONFIG_DISPLAYREGIONTYPE current)
{
  printf("*********************************************\n");
  printf("nSize = %d\n", current.nSize );
  printf("nVersion.nVersion = %d\n", current.nVersion.nVersion );
  printf("nPortIndex = %d\n", current.nPortIndex );

  putchar('\n');
  printf("set = ");
  printBits(&current.set);
  putchar('\n');
  
  printf("OMX_DISPLAY_SET_NONE = %d\n", (current.set >> 0) & 1);
  printf("OMX_DISPLAY_SET_NUM = %d\n", (current.set >> 1) & 1);
  printf("OMX_DISPLAY_SET_FULLSCREEN = %d\n", (current.set >> 2) & 1);
  printf("OMX_DISPLAY_SET_TRANSFORM = %d\n", (current.set >> 3) & 1);
  printf("OMX_DISPLAY_SET_DEST_RECT = %d\n", (current.set >> 4) & 1);
  printf("OMX_DISPLAY_SET_SRC_RECT = %d\n", (current.set >> 5) & 1);
  printf("OMX_DISPLAY_SET_MODE = %d\n", (current.set >> 6) & 1);
  printf("OMX_DISPLAY_SET_PIXEL = %d\n", (current.set >> 7) & 1);
  printf("OMX_DISPLAY_SET_NOASPECT = %d\n", (current.set >> 8) & 1);
  printf("OMX_DISPLAY_SET_LAYER = %d\n", (current.set >> 9) & 1);
  printf("OMX_DISPLAY_SET_COPYPROTECT = %d\n", (current.set >> 10) & 1);
  printf("OMX_DISPLAY_SET_ALPHA = %d\n", (current.set >> 11) & 1);
  printf("OMX_DISPLAY_SET_DUMMY = %d\n", current.set == OMX_DISPLAY_SET_DUMMY);

  putchar('\n');
  
  printf("num = %d\n", current.num );
  printf("fullscreen = %d\n", current.fullscreen );

  printf("transform = ");
  switch(current.transform)
    {
    case 0: printf("OMX_DISPLAY_ROT0   (0)\n"); break;
    case 1: printf("OMX_DISPLAY_MIRROR_ROT0   (1)\n"); break;
    case 2: printf("OMX_DISPLAY_MIRROR_ROT180   (2)\n"); break;
    case 3: printf("OMX_DISPLAY_ROT180   (3)\n"); break;
    case 4: printf("OMX_DISPLAY_MIRROR_ROT90   (4)\n"); break;
    case 5: printf("OMX_DISPLAY_ROT270   (5)\n"); break;
    case 6: printf("OMX_DISPLAY_ROT90   (6)\n"); break;
    case 7: printf("OMX_DISPLAY_MIRROR_ROT270   (7)\n"); break;
    case 0x7FFFFFFF: printf("OMX_DISPLAY_DUMMY   (0x7FFFFFFF)\n"); break;
    default : printf("Error: Enumeration unknown"); break;
    }

  putchar('\n');  

  printf("dest_rect\n");
  printf("\tdest_rect.x_offset = %d\n", current.dest_rect.x_offset);
  printf("\tdest_rect.y_offset = %d\n", current.dest_rect.y_offset);
  printf("\tdest_rext.width = %d\n", current.dest_rect.width);
  printf("\tdest_rext.height = %d\n", current.dest_rect.height);

  putchar('\n');  

  printf("src_rect\n");
  printf("\tsrc_rect.x_offset = %d\n", current.src_rect.x_offset);
  printf("\tsrc_rect.y_offset = %d\n", current.src_rect.y_offset);
  printf("\tsrc_rext.width = %d\n", current.src_rect.width);
  printf("\tsrc_rext.height = %d\n", current.src_rect.height);
  
  putchar('\n');  

  printf("noaspect = %d\n", current.noaspect );
  printf("pixel_x = %d\n", current.pixel_x );
  printf("pixel_y = %d\n", current.pixel_y );
  printf("layer = %d\n", current.layer );
  printf("copyprotect_required = %d\n", current.copyprotect_required );
  printf("alpha = %d\n", current.alpha );
  printf("wfc_contect_width = %d\n", current.wfc_context_width );
  printf("wfc_context_height = %d\n", current.wfc_context_height );

  printf("*********************************************\n\n");
  
}

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


void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data)
{
  fprintf(stderr, "OMX error %s\n", err2str(data));
}

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

#include "bcm_host.h"
#include "ilclient.h"


/////////////////////////////////////////////////////////////////
// MACROS
/////////////////////////////////////////////////////////////////

// MACRO to initialize structures
#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.nVersion = OMX_VERSION;


/////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
/////////////////////////////////////////////////////////////////

void printState(OMX_HANDLETYPE handle);

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

  COMPONENT_T *camera = NULL;
  OMX_ERRORTYPE OMXstatus;
  uint32_t screen_width = 0, screen_height = 0;
  OMX_BUFFERHEADERTYPE *camera_out;
  int count;

  //port param preview stucture
  OMX_PARAM_PORTDEFINITIONTYPE port_params;
  OMX_INIT_STRUCTURE(port_params);
  
  
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
  
  //initialise camera
  ilclient_create_component(client,
			    &camera,
			    "camera",
			    ILCLIENT_DISABLE_ALL_PORTS
			    | ILCLIENT_ENABLE_OUTPUT_BUFFERS);
  
  
  printState(ilclient_get_handle(camera));

  //change the preview res
  port_params.nPortIndex = 70;
  OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter. Error = %s\n", err2str(OMXstatus));

  port_params.format.video.nFrameWidth = 320;
  port_params.format.video.nFrameHeight = 240;
  port_params.format.video.nStride = 320;
  port_params.format.video.nSliceHeight = 240;
  port_params.format.video.xFramerate = 24 << 16;

  OMXstatus = OMX_SetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter. Error = %s\n", err2str(OMXstatus));


  
  OMXstatus = ilclient_change_component_state(camera, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move camera component to Idle (1)");
      exit(EXIT_FAILURE);
    }
  
  ilclient_enable_port_buffers(camera, 70, NULL, NULL, NULL);
  ilclient_enable_port(camera, 70);

  ilclient_enable_port_buffers(camera, 72, NULL, NULL, NULL);
  ilclient_enable_port(camera, 72);

  printState(ilclient_get_handle(camera));

  /////////////////////////////////////////////////////////////////
  // Main Meat
  /////////////////////////////////////////////////////////////////
  
  // change camera component to executing
  OMXstatus = ilclient_change_component_state(camera, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move camera component to Executing (1)\n");
      exit(EXIT_FAILURE);
    }
  printState(ilclient_get_handle(camera));

  for(count = 0; count < 100; count++)
    {
      camera_out = ilclient_get_output_buffer(camera, 70, 1/*block*/);
      printf("nFilledLen = %d\n", camera_out->nFilledLen);
    }

  printf("merp\n");

  /////////////////////////////////////////////////////////////////
  //CLEANUP
  /////////////////////////////////////////////////////////////////

  OMXstatus = ilclient_change_component_state(camera, OMX_StateIdle);


  //close files
  
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

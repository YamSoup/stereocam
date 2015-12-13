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

  COMPONENT_T *camera = NULL, *video_render = NULL, *image_encode = NULL;
  OMX_ERRORTYPE OMXstatus;
  uint32_t screen_width = 0, screen_height = 0;
  OMX_BUFFERHEADERTYPE *decode_out;

  FILE *file_out1;
  file_out1 = fopen("pic1", "wb");
  
  TUNNEL_T tunnel_camera_to_render, tunnel_camera_to_encode;
  memset(&tunnel_camera_to_render, 0, sizeof(tunnel_camera_to_render));
  memset(&tunnel_camera_to_encode, 0, sizeof(tunnel_camera_to_encode));

  OMX_CONFIG_DISPLAYREGIONTYPE render_config;
  memset(&render_config, 0, sizeof(render_config));
  render_config.nVersion.nVersion = OMX_VERSION;
  render_config.nSize = sizeof(render_config);
  render_config.nPortIndex = 90; 

  OMX_FRAMESIZETYPE frame_size;
  memset(&frame_size, 0, sizeof(frame_size));
  frame_size.nVersion.nVersion = OMX_VERSION;
  frame_size.nSize = sizeof(frame_size);
  frame_size.nPortIndex = 72;

  //camera still capture stuctures
  OMX_IMAGE_PARAM_PORTFORMATTYPE image_format;
  memset(&image_format, 0, sizeof(image_format));
  image_format.nVersion.nVersion = OMX_VERSION;
  image_format.nSize = sizeof(image_format);
  
  OMX_CONFIG_PORTBOOLEANTYPE still_capture_in_progress;
  memset(&still_capture_in_progress, 0, sizeof(still_capture_in_progress));
  still_capture_in_progress.nVersion.nVersion = OMX_VERSION;
  still_capture_in_progress.nSize = sizeof(still_capture_in_progress);
  still_capture_in_progress.nPortIndex = 72;
  still_capture_in_progress.bEnabled = 0;
  
  
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
			    ILCLIENT_DISABLE_ALL_PORTS);
  
  OMXstatus = ilclient_change_component_state(camera, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move camera component to Idle (1)");
      exit(EXIT_FAILURE);
    }
  // change the preview resolution of the camera
  //


  //Initialise video render
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

  /*
  OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamPortMaxFrameSize, &frame_size);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error getting Paramter. Error = %s\n", err2str(OMXstatus));
  */
  //camera preview and render window at (accounting for 16x16 limitations)
  // change the render to display in a portion of the screen

  //Initalise Image Encoder
  ilclient_create_component(client,
			    &image_encode,
			    "image_encode",
			    ILCLIENT_DISABLE_ALL_PORTS
			    /*| ILCLIENT_ENABLE_INPUT_BUFFERS*/
			    | ILCLIENT_ENABLE_OUTPUT_BUFFERS);
  printState(ilclient_get_handle(image_encode));

  OMXstatus = ilclient_change_component_state(image_encode, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move image encode component to Idle (1)\n");
      exit(EXIT_FAILURE);
    }
  printState(ilclient_get_handle(image_encode));

  //populate image_format with information from the camera port
  image_format.nPortIndex = 72;

  OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamImagePortFormat, &image_format);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error Getting Paramter. Error = %s\n", err2str(OMXstatus));

  //change some settings and set parameters
  image_format.eCompressionFormat = OMX_IMAGE_CodingJPEG;
  image_format.nPortIndex = 341;
  image_format.nVersion.nVersion = OMX_VERSION;
  image_format.nSize = sizeof(image_format);
  
  OMXstatus = OMX_SetParameter(ilclient_get_handle(image_encode), OMX_IndexParamImagePortFormat, &image_format);
  if(OMXstatus != OMX_ErrorNone)
    printf("Error setting Paramter. Error = %s\n", err2str(OMXstatus));

  /////////////////////////////////////////////////////////////////
  // Main Meat
  /////////////////////////////////////////////////////////////////
  
  //setup tunnel of camera preview to renderer
  set_tunnel(&tunnel_camera_to_render, camera, 70, video_render, 90);
  ilclient_setup_tunnel(&tunnel_camera_to_render, 0, 0);
  
  // change camera component to executing
  OMXstatus = ilclient_change_component_state(camera, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move camera component to Executing (1)\n");
      exit(EXIT_FAILURE);
    }
  printState(ilclient_get_handle(camera));

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

  //enable port and buffers for output por of image encode
  ilclient_enable_port_buffers(image_encode, 341, NULL, NULL, NULL);  
  ilclient_enable_port(image_encode, 341);

  //setup tunnel from camera image port too image encode
  set_tunnel(&tunnel_camera_to_encode, camera, 72, image_encode, 340);    
  ilclient_setup_tunnel(&tunnel_camera_to_encode, 0, 0);

  //change image_encode to executing
  OMXstatus = ilclient_change_component_state(image_encode, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move image_encode component to Executing (1) Error = %s\n", err2str(OMXstatus));
      exit(EXIT_FAILURE);
    }
  printState(ilclient_get_handle(image_encode));

  
  //tell API port is taking picture
  still_capture_in_progress.bEnabled = 1;
  OMXstatus = OMX_SetConfig(ilclient_get_handle(camera),
			       OMX_IndexConfigPortCapturing,
			       &still_capture_in_progress);  
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to set Config (1)\n");
      exit(EXIT_FAILURE);
    }  

  while(1)
    {
      decode_out = ilclient_get_output_buffer(image_encode, 341, 1/*blocking*/);
      printf("decode_out bytes = %d\n", decode_out->nFilledLen);
      printf("decode_out bufferflags = %d\n\n", decode_out->nFlags);
      if(decode_out->nFilledLen != 0) 
	fwrite(decode_out->pBuffer, 1, decode_out->nFilledLen, file_out1);
      if(decode_out->nFlags == 1)
	break;
      OMX_FillThisBuffer(ilclient_get_handle(image_encode), decode_out);
    }
    
  
  //tell API port is finished
  still_capture_in_progress.bEnabled = 0;
  still_capture_in_progress.nVersion.nVersion = OMX_VERSION;
  still_capture_in_progress.nSize = sizeof(still_capture_in_progress);
  still_capture_in_progress.nPortIndex = 72;
  
  OMXstatus = OMX_SetConfig(ilclient_get_handle(camera),
			       OMX_IndexConfigPortCapturing,
			       &still_capture_in_progress);  
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to set Config (1)\n");
      exit(EXIT_FAILURE);
    }
  
  
  //sleep for 2 secs
  sleep(2);
  printf("merp\n");

  /////////////////////////////////////////////////////////////////
  //CLEANUP
  /////////////////////////////////////////////////////////////////

  //close files
  fclose(file_out1);
  
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

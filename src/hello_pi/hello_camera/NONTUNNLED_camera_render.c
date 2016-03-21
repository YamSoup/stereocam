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

  COMPONENT_T *camera = NULL, *video_render = NULL;
  OMX_ERRORTYPE OMXstatus;
  uint32_t screen_width = 0, screen_height = 0;
  OMX_BUFFERHEADERTYPE *camera_out, *video_in;
  int count;
  
  
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

  //////////////////////////////////  
  //initialise camera
  ilclient_create_component(client,
			    &camera,
			    "camera",
			    ILCLIENT_DISABLE_ALL_PORTS
			    | ILCLIENT_ENABLE_OUTPUT_BUFFERS);
  
  OMXstatus = ilclient_change_component_state(camera, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move camera component to Idle (1)");
      exit(EXIT_FAILURE);
    }

  //port param preview stucture
  OMX_PARAM_PORTDEFINITIONTYPE camera_params;
  OMX_INIT_STRUCTURE(camera_params);
  camera_params.nPortIndex = 70;

  OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &camera_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter. Error = %s\n", err2str(OMXstatus));

  camera_params.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
  camera_params.format.video.nFrameWidth = 320;
  camera_params.format.video.nFrameHeight = 240;
  camera_params.format.video.nStride = 320;
  camera_params.format.video.nSliceHeight = 240;
  camera_params.format.video.xFramerate = 24 << 16;

  OMXstatus = OMX_SetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &camera_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter. Error = %s\n", err2str(OMXstatus));

  //check buffer 
  OMXstatus = OMX_GetParameter(ilclient_get_handle(camera), OMX_IndexParamPortDefinition, &camera_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter. Error = %s\n", err2str(OMXstatus));

  printf("camera nBufferSize = %d\n", camera_params.nBufferSize);
  printf("camera nBufferCountMin = %d, nBufferCountActual = %d\n", camera_params.nBufferCountMin, camera_params.nBufferCountActual);

  printf("enable camera output port ");
  ilclient_enable_port_buffers(camera, 70, NULL, NULL, NULL);
  ilclient_enable_port(camera, 70);

  printf("Camera state = ");
  printState(ilclient_get_handle(camera));


  /////////////////////////////////
  // initialze video render

  ilclient_create_component(client,
			    &video_render,
			    "video_render",
			    ILCLIENT_DISABLE_ALL_PORTS
			    | ILCLIENT_ENABLE_INPUT_BUFFERS);
  
  OMXstatus = ilclient_change_component_state(video_render, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move video_render component to Idle (1)");
      exit(EXIT_FAILURE);
    }
  printState(ilclient_get_handle(video_render));

  //render param structure
  OMX_PARAM_PORTDEFINITIONTYPE render_params;
  OMX_INIT_STRUCTURE(render_params);
  render_params.nPortIndex = 90;
  
  OMXstatus = OMX_GetParameter(ilclient_get_handle(video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter(in video render). Error = %s\n", err2str(OMXstatus));

  render_params.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
  render_params.format.video.nFrameWidth = 320;
  render_params.format.video.nFrameHeight = 240;
  render_params.format.video.nStride = 320;
  render_params.format.video.nSliceHeight = 240;
  render_params.format.video.xFramerate = 24 << 16;

  OMXstatus = OMX_SetParameter(ilclient_get_handle(video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter(in video render). Error = %s\n", err2str(OMXstatus));


  //check buffers
  OMXstatus = OMX_GetParameter(ilclient_get_handle(video_render), OMX_IndexParamPortDefinition, &render_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter(in video render). Error = %s\n", err2str(OMXstatus));

  printf("nBufferCountMin = %d, nBufferCountActual = %d\n", render_params.nBufferCountMin,
	 render_params.nBufferCountActual);

  printf("enable video_render input port ");
  ilclient_enable_port_buffers(video_render, 90, NULL, NULL, NULL);
  ilclient_enable_port(video_render, 90);
  

  /////////////////////////////////////////////////////////////////
  // Main Meat
  /////////////////////////////////////////////////////////////////
  
  // change camera component to executing
  OMXstatus = ilclient_change_component_state(camera, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move camera component to Executing (5)\n");
      exit(EXIT_FAILURE);
    }
  printf("camera is "); printState(ilclient_get_handle(camera));

  // change render component to executing
  OMXstatus = ilclient_change_component_state(video_render, OMX_StateExecuting);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move video_render component to Executing (5)\n");
      exit(EXIT_FAILURE);
    }
  printf("video_render is "); printState(ilclient_get_handle(video_render));




  for(count = 0; count < 100; count++)
    {
      //get output buffer from camera
      OMX_FillThisBuffer(ilclient_get_handle(camera), camera_out);      
      camera_out = ilclient_get_output_buffer(camera, 70, 1);

      if(camera_out != NULL)
	{
	  printf("count = %d\n", count);
	  printf("nFilledLen = %d\n", camera_out->nFilledLen);
	  printf("camera_out pBuffer address  = %#010x\n", (unsigned int)camera_out->pBuffer);
	}
      else
	printf("NULL\n");
      
      //get input buffer from renderer
      video_in = ilclient_get_input_buffer(video_render, 90, 1);
      printf("video_in pBuffer address = %#10x\n", (unsigned int)video_in->pBuffer);

      //copy the buffer information from 1 buffer to the other
      memcpy(video_in->pBuffer, camera_out->pBuffer, render_params.nBufferSize);

      OMX_EmptyThisBuffer(ilclient_get_handle(video_render), video_in);
      
    }

  printf("end\n");

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


/*
int use_buffer(COMPONENT_T *renderComponent, 
	       OMX_BUFFERHEADERTYPE *buff_header) {
  int ret;
  OMX_PARAM_PORTDEFINITIONTYPE portdef;

    ppRenderInputBufferHeader =
      (OMX_BUFFERHEADERTYPE **) malloc(sizeof(void) *
				       3);

    OMX_SendCommand(ilclient_get_handle(renderComponent), 
		    OMX_CommandPortEnable, 90, NULL);
    
    ilclient_wait_for_event(renderComponent,
			    OMX_EventCmdComplete,
			    OMX_CommandPortEnable, 1,
			    90, 1, 0,
			    5000);
    
    printState(ilclient_get_handle(renderComponent));

    ret = OMX_UseBuffer(ilclient_get_handle(renderComponent),
			&ppRenderInputBufferHeader[0],
			90,
			NULL,
			nBufferSize,
			buff_header->pBuffer);
    if (ret != OMX_ErrorNone) {
      fprintf(stderr, "Eror sharing buffer %s\n", err2str(ret));
      return ret;
    } else {
      printf("Sharing buffer ok\n");
    }

    ppRenderInputBufferHeader[0]->nAllocLen =
      buff_header->nAllocLen;

    int n;
    for (n = 1; n < 3; n++) {
      printState(ilclient_get_handle(renderComponent));
      ret = OMX_UseBuffer(ilclient_get_handle(renderComponent),
			  &ppRenderInputBufferHeader[n],
			  90,
			  NULL,
			  0,
			  NULL);
      if (ret != OMX_ErrorNone) {
	fprintf(stderr, "Eror sharing null buffer %s\n", err2str(ret));
	return ret;
      }
    }

}
*/

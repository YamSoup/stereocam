/*
  http://www.jvcref.com/files/PI/documentation/ilcomponents/index.html
  https://github.com/SonienTaegi/rpi-omx-tutorial/blob/master/camera_tunnel_non.c
  http://maemo.org/api_refs/5.0/beta/libomxil-bellagio/_o_m_x___index_8h.html 
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>

#include "bcm_host.h"
#include "ilclient.h"

#include "IL/OMX_Core.h"
#include "IL/OMX_Component.h"
#include "IL/OMX_Video.h"
#include "IL/OMX_Broadcom.h"


/////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
/////////////////////////////////////////////////////////////////


OMX_ERRORTYPE onOMXevent ( OMX_IN OMX_HANDLETYPE hComponent,
			   OMX_IN OMX_PTR pAppData,
			   OMX_IN OMX_EVENTTYPE eEvent,
			   OMX_IN OMX_U32 nData1,
			   OMX_IN OMX_U32 nData2,
			   OMX_IN OMX_PTR pEventData);

OMX_ERRORTYPE onFillCameraOut ( OMX_OUT OMX_HANDLETYPE hComponent,
				OMX_OUT OMX_PTR pAppData,
				OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE onEmptyRenderIn ( OMX_IN OMX_HANDLETYPE hComponent,
				OMX_IN OMX_PTR pAppData,
				OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

void printBits(void *toPrint);

void printState(OMX_HANDLETYPE handle);

void print_OMX_CONFIG_DISPLAYREGIONTYPE(OMX_CONFIG_DISPLAYREGIONTYPE current);

char *err2str(int err);

void error_callback(void *userdata, COMPONENT_T *comp, OMX_U32 data); 

// define stuct for callbacks
typedef struct {
  OMX_BOOL isCameraReady;
  OMX_BOOL isFilled;
} CONTEXT;
//create instance of the stuct
CONTEXT mContext;


/////////////////////////////////////////////////////////////////
// MAIN
/////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  /////////////////////////////////////////////////////////////////
  // VARIABLES
  /////////////////////////////////////////////////////////////////

  ILCLIENT_T *client;
  OMX_ERRORTYPE OMXstatus;
  uint32_t screen_width = 0, screen_height = 0;
  
  /////////////////////////////////////////////////////////////////
  // STARTUP
  /////////////////////////////////////////////////////////////////

  //initialise bcm_host
  bcm_host_init();

  //initalize OMX
  OMXstatus = OMX_Init();
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to initalize OMX");
      ilclient_destroy(client);
      exit(EXIT_FAILURE);
    }

  //super special broadcom only function
  graphics_get_display_size(0/*framebuffer 0*/, &screen_width, &screen_height);
  printf("screen_width = %d, screen_height = %d\n", (int)screen_width, (int)screen_height);
  
  
  /////////////////////////////////////////////////////////////////
  // Initalize Components
  /////////////////////////////////////////////////////////////////
  
  ///////////////////////////////////////////
  ////initialise camera////
  ///////////////////////////////////////////
  
  OMX_HANDLETYPE camera = NULL;  

  OMX_CALLBACKTYPE callbackOMX;
  callbackOMX.EventHandler = onOMXevent;
  callbackOMX.EmptyBufferDone = onEmptyRenderIn;
  callbackOMX.FillBufferDone = onFillCameraOut;

  //get handle (create component)
  OMXstatus = OMX_GetHandle(&camera,
			    "OMX.broadcom.camera",
			    &client,
			    &callbackOMX
			    );

  if (OMXstatus != OMX_ErrorNone)
    {
      printf("Error creating camera\n");
      exit(EXIT_FAILURE);
    }
  else
    printf("camera creation worked?\n");

  OMX_SendCommand(camera, OMX_CommandPortDisable, 70, NULL);  
  OMX_SendCommand(camera, OMX_CommandPortDisable, 72, NULL);  
  OMX_SendCommand(camera, OMX_CommandPortDisable, 73, NULL);  


  //Configure OMX_IndexParamCameraDeviceNumber callback (to check if camera is initalize correctly)
  OMX_CONFIG_REQUESTCALLBACKTYPE configCameraCallback;
  configCameraCallback.nSize = sizeof(configCameraCallback);
  configCameraCallback.nVersion.nVersion = OMX_VERSION;

  configCameraCallback.nPortIndex = OMX_ALL;
  configCameraCallback.nIndex = OMX_IndexParamCameraDeviceNumber;
  configCameraCallback.bEnable = OMX_TRUE;

  OMXstatus = OMX_SetConfig(camera, OMX_IndexConfigRequestCallback, &configCameraCallback);
  if (OMXstatus != OMX_ErrorNone)
    {
      printf("Error setting camera callback");
      exit(EXIT_FAILURE);
    }

  //set OMX CameraDeviceNumber
  OMX_PARAM_U32TYPE deviceNumber;
  deviceNumber.nSize = sizeof(deviceNumber);
  deviceNumber.nVersion.nVersion = OMX_VERSION;

  deviceNumber.nPortIndex = OMX_ALL;
  deviceNumber.nU32 = 0;
  
  OMXstatus = OMX_SetParameter(camera, OMX_IndexParamCameraDeviceNumber, &deviceNumber);
  if (OMXstatus != OMX_ErrorNone)
    {
      printf("Error setting device number");
      exit(EXIT_FAILURE);
    }



    ////////////////
  
  OMX_PARAM_PORTDEFINITIONTYPE port_params;
  memset(&port_params, 0, sizeof(port_params));
  port_params.nVersion.nVersion = OMX_VERSION;
  port_params.nSize = sizeof(port_params);
  port_params.nPortIndex = 70;
  //prepopulate structure
  OMXstatus = OMX_GetParameter(camera, OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter. Error = %s\n", err2str(OMXstatus));

  
  //change needed params
  port_params.format.video.nFrameWidth = 320;
  port_params.format.video.nFrameHeight = 240;
  port_params.format.video.nStride = 320;
  port_params.format.video.nSliceHeight = 240;
  port_params.format.video.nBitrate = 0;
  port_params.format.video.xFramerate = 24;
  port_params.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
  port_params.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  

  //set changes
  OMXstatus = OMX_SetParameter(camera, OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Setting Parameter. Error = %s\n", err2str(OMXstatus));
    
  
  //allocate buffer(s)
  memset(&port_params, 0, sizeof(port_params));
  port_params.nVersion.nVersion = OMX_VERSION;
  port_params.nSize = sizeof(port_params);
  port_params.nPortIndex = 70;
  
  OMXstatus = OMX_GetParameter(camera, OMX_IndexParamPortDefinition, &port_params);
  if (OMXstatus != OMX_ErrorNone)
    printf("Error Getting Parameter. Error = %s\n", err2str(OMXstatus));


  printf("nSize = %d,   nPortIndex = %d,   eDir = %d,   bEnabled = %d\n",
	 port_params.nSize,
	 port_params.nPortIndex,
	 port_params.eDir, 
	 port_params.bEnabled);

	 
  printf("buffer count = %d   buffer size = %d\n",
	 port_params.nBufferCountActual,
	 port_params.nBufferSize);

  printf("xFramerate = %d, nBitrate = %d\n",
	 port_params.format.video.xFramerate,
	 port_params.format.video.nBitrate);
  
  while(!mContext.isCameraReady)
    {
      printf("Waiting for camera to be ready");
      usleep(100 *1000);
    }
  
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

/**********************************
FUNCTIONS
**********************************/


/* print event (used in callbacks)*/
void print_event(OMX_HANDLETYPE hComponent, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2) {
  char *e;
  switch(eEvent) {
  case OMX_EventCmdComplete:
    e = "CmdComplete";
    break;
  case OMX_EventError:
    e = "Error";
    break;
  case OMX_EventMark:
    e = "Mark";
    break;
  case OMX_EventPortSettingsChanged:
    e = "PortSettingsChanged";
    break;
  case OMX_EventBufferFlag:
    e = "BufferFlag";
    break;
  case OMX_EventResourcesAcquired:
    e = "ResourcesAcquired";
    break;
  case OMX_EventComponentResumed:
    e = "ComponentResumed";
    break;
  case OMX_EventDynamicResourcesAvailable:
    e = "DynamicResourcesAvailable";
    break;
  case OMX_EventParamOrConfigChanged:
    e = "ParamOrConfigChanged";
    break;
  default:
    e = "Others";
  }
  printf("EVENT [0x%08x] @0x%08x : %s (nData1=0x%08x, nData2=0x%08x)\n", (int)eEvent, (int)hComponent, e, nData1, nData2);
}

/* Event Handler : OMX Event */
OMX_ERRORTYPE onOMXevent (
			  OMX_IN OMX_HANDLETYPE hComponent,
			  OMX_IN OMX_PTR pAppData,
			  OMX_IN OMX_EVENTTYPE eEvent,
			  OMX_IN OMX_U32 nData1,
			  OMX_IN OMX_U32 nData2,
			  OMX_IN OMX_PTR pEventData) {

  print_event(hComponent, eEvent, nData1, nData2);

  switch(eEvent) {
  case OMX_EventParamOrConfigChanged :
    if(nData2 == OMX_IndexParamCameraDeviceNumber) {
      ((CONTEXT*)pAppData)->isCameraReady = OMX_TRUE;
      printf("Camera device is ready.");
    }
    break;
  default :
    break;
  }
  return OMX_ErrorNone;
}

/* Callback : Camera-out buffer is filled */
OMX_ERRORTYPE onFillCameraOut (
			       OMX_OUT OMX_HANDLETYPE hComponent,
			       OMX_OUT OMX_PTR pAppData,
			       OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer) {
  mContext.isFilled = OMX_TRUE;
  return OMX_ErrorNone;
}

/* Callback : Render-in buffer is emptied */
OMX_ERRORTYPE onEmptyRenderIn(
			      OMX_IN OMX_HANDLETYPE hComponent,
			      OMX_IN OMX_PTR pAppData,
			      OMX_IN OMX_BUFFERHEADERTYPE* pBuffer) {

  printf("BUFFER 0x%08x emptied\n", (int)pBuffer);
  return OMX_ErrorNone;
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

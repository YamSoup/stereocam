#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bcm_host.h"
#include "ilclient.h"

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

int get_file_size(char *fname) {
  struct stat st;

  if (stat(fname, &st) == -1) {
    perror("Stat'ing img file");
    return -1;
  }
  return(st.st_size);
}

/////////////////////////////////////////////////////////////////
// MAIN
/////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  /////////////////////////////////////////////////////////////////
  // VARIABLES
  /////////////////////////////////////////////////////////////////

  ILCLIENT_T *client;
  FILE *file_in;

  COMPONENT_T *video_decode = NULL, *video_render = NULL;
  TUNNEL_T tunnel_decode_to_render;
  OMX_ERRORTYPE OMXstatus;
  
  memset(&tunnel_decode_to_render, 0, sizeof(tunnel_decode_to_render));

  /////////////////////////////////////////////////////////////////
  // STARTUP
  /////////////////////////////////////////////////////////////////

  //initialise bcm_host
  bcm_host_init();

  //check user has entered a file
  if(argc != 2)
    {
      printf("usage: camera <filename>\n");
      exit(EXIT_FAILURE);
    }
  
  //try and open file
  file_in = fopen(argv[1], "rb");
  if(file_in == NULL)
    {
      fprintf(stderr, "unable to open requested file");
      exit(EXIT_FAILURE);
    }
  
  //create client
  client = ilclient_init();
  if(client == NULL)
    {
      fprintf(stderr, "unable to initalize ilclient\n");
      fclose(file_in);
      exit(EXIT_FAILURE);
    }

  //initalize OMX
  OMXstatus = OMX_Init();
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to initalize OMX");
      fclose(file_in);
      ilclient_destroy(client);
      exit(EXIT_FAILURE);
    }
  
  /////////////////////////////////////////////////////////////////
  // Initalize Components
  /////////////////////////////////////////////////////////////////

  //create video decode component /////////////////////////////////
  ilclient_create_component(client,
			    &video_decode,
			    "video_decode",
			    ILCLIENT_DISABLE_ALL_PORTS
			    | ILCLIENT_ENABLE_INPUT_BUFFERS);
  // create structure for the video port format
  OMX_VIDEO_PARAM_PORTFORMATTYPE videoPortFormat;
  //0 the parameter structure
  memset(&videoPortFormat, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
  // modify the parameter structure
  videoPortFormat.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
  videoPortFormat.nVersion.nVersion = OMX_VERSION;
  videoPortFormat.nPortIndex = 130;
  videoPortFormat.eCompressionFormat = OMX_VIDEO_CodingAVC;
  // set the modified parameters
  OMXstatus = OMX_SetParameter(ilclient_get_handle(video_decode), OMX_IndexParamVideoPortFormat, &videoPortFormat);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to set decoder parameters error = %s", err2str(OMXstatus));
      exit(EXIT_FAILURE);
    }

  //change state of component to idle
  OMXstatus = ilclient_change_component_state(video_decode, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move component to Idle");
      exit(EXIT_FAILURE);
    }    

  //create video render component /////////////////////////////////
  ilclient_create_component(client,
			    &video_render,
			    "video_render",
			    ILCLIENT_DISABLE_ALL_PORTS);
  //!!
  //OMX_CONFIG_DISPLAYREGIONTYPE !!
  //!!

  //change state of render component to idle
  OMXstatus = ilclient_change_component_state(video_render, OMX_StateIdle);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to move component to Idle");
      exit(EXIT_FAILURE);
    }

  // set tunnel
  set_tunnel(&tunnel_decode_to_render, video_decode, 131, video_render, 90);

  /////////////////////////////////////////////////////////////////
  // MAIN MEAT
  /////////////////////////////////////////////////////////////////
  
  //enable port buffers
  OMXstatus = ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL);
  if (OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "Error enabling port buffers for decodes, Error %s", err2str(OMXstatus));
      exit(EXIT_FAILURE);
    }
  //enable port
  ilclient_enable_port(video_decode, 130);

  //change the decoder component to state executing
  OMXstatus = ilclient_change_component_state(video_decode, OMX_StateExecuting);
  if(OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "unable to change decoder state to Executing");
      exit(EXIT_FAILURE);
    }
  //check state
  printState(ilclient_get_handle(video_decode));

  OMX_BUFFERHEADERTYPE *buffer_header = NULL;
  int toread = get_file_size(argv[1]);
  int current_read = 0, buffer_size = 0;
  int result = -1;
  //feed and empty buffers in until port format changes

  while (toread > 0)
    {      
      buffer_header = ilclient_get_input_buffer(video_decode, 130, 1 /* block */);     
      if(buffer_header == NULL)
	{
	  printf("buffer_header is NULL\n");
	  exit(EXIT_FAILURE);
	}
      buffer_size = buffer_header->nAllocLen;
      current_read = fread(buffer_header->pBuffer, 1, buffer_size, file_in);
      buffer_header->nFilledLen = current_read;
      toread -= current_read;
      printf("Read %d, %d still left to read\n", current_read, toread);
    
      OMXstatus = OMX_EmptyThisBuffer(ilclient_get_handle(video_decode), buffer_header);
      if (OMXstatus != OMX_ErrorNone)
	{
	  fprintf(stderr, "unable to empty buffer in format change loop, error %s", err2str(OMXstatus));
	  exit(EXIT_FAILURE);
	}
    
      result = ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1);
      if(result == 0)
	{
	  printf("port changed sucsess\n");
	  break;
	}
      
      if (toread <= 0)
	buffer_header->nFlags |= OMX_BUFFERFLAG_EOS;
    }
  
  //setup tunnel
  ilclient_setup_tunnel(&tunnel_decode_to_render, 0, 0);

  OMXstatus = ilclient_change_component_state(video_render, OMX_StateExecuting);
  if(OMXstatus != OMX_ErrorNone)
    {
      fprintf(stderr, "Unable to change render component to executeing");
      exit(EXIT_FAILURE);
    }
  
  

  //start reading the rest of the data 
  while(toread > 0)
    {
      buffer_header = ilclient_get_input_buffer(video_decode, 130, 1 /* block */);     
      if(buffer_header == NULL)
	{
	  printf("buffer_header is NULL\n");
	  exit(EXIT_FAILURE);
	}
      buffer_size = buffer_header->nAllocLen;
      current_read = fread(buffer_header->pBuffer, 1, buffer_size, file_in);
      buffer_header->nFilledLen = current_read;
      toread -= current_read;
      printf("Read %d, %d still left to read\n", current_read, toread);
    
      if (toread <= 0)
	buffer_header->nFlags |= OMX_BUFFERFLAG_EOS;

      OMXstatus = OMX_EmptyThisBuffer(ilclient_get_handle(video_decode), buffer_header);
      if (OMXstatus != OMX_ErrorNone)
	{
	  fprintf(stderr, "unable to empty buffer in format change loop, error %s", err2str(OMXstatus));
	  exit(EXIT_FAILURE);
	}
    }

   ilclient_wait_for_event(video_render,
			   OMX_EventBufferFlag,
			   90, 0,
			   0, 1,
			   OMX_BUFFERFLAG_EOS,
			   2000); 

  /////////////////////////////////////////////////////////////////
  //CLEANUP
  /////////////////////////////////////////////////////////////////

  //Disable components
  

  //check all components have been cleaned up
  OMX_Deinit();

  //destroy client
  ilclient_destroy(client);

  //close file
  fclose(file_in);

  return 0;
}


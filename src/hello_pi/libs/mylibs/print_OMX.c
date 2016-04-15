

void print_OMX_AUDIO_PORTDEFINITIONTYPE(OMX_AUDIO_PORTDEFINITIONTYPE audio)
{
  /*
  printf("   audio.cMIMEType = %s\n", audio.cMIMEType);
  printf("   audio.pNativeRender = %d\n", (int)audio.pNativeRender);
  printf("   audio.bFlagErrorConcealment = ");
  audio.bFlagErrorConcealment ? printf("true\n") : printf("false\n");
  printf("   audio.eEncoding = %d (enumeration not implemented)\n", (int)audio.eEncoding);
  */
  printf("Not implemented yet\n");
}

void print_OMX_VIDEO_PORTDEFINITIONTYPE(OMX_VIDEO_PORTDEFINITIONTYPE video)
{
  printf("OMX_VIDEO_PORT_DEFINITIONTYPE\n");
  printf("   .video.cMIMEType = %s\n", video.cMIMEType);
  printf("   .video.pNativeRender = %d ??\n", (int)video.pNativeRender);
  printf("   .video.nFrameWidth = %d\n", (int)video.nFrameWidth);
  printf("   .video.nFrameHeight = %d\n", (int)video.nFrameHeight);
  printf("   .video.nStride = %d\n", (int)video.nStride);
  printf("   .video.nSliceHeight = %d\n", (int)video.nSliceHeight);
  printf("   .video.nBitrate = %d\n", (int)video.nBitrate);
  printf("   .video.xFramerate >> 16 = %d\n", (int)video.xFramerate >>16 );
  printf("   .video.bFlagErrorConcealment = ");
  video.bFlagErrorConcealment ? printf("true\n") : printf("false\n");
  printf("   .video.eCompressionFormat = ");
  switch(video.eCompressionFormat)
    {
    case OMX_VIDEO_CodingUnused:     printf("OMX_VIDEO_CodingUnused\n"); break;
    case OMX_VIDEO_CodingAutoDetect: printf("OMX_VIDEO_CodingAutoDetect\n"); break;
    case OMX_VIDEO_CodingMPEG2:      printf("OMX_VIDEO_CodingMPEG2\n"); break;
    case OMX_VIDEO_CodingH263:       printf("OMX_VIDEO_CodingH263\n"); break;
    case OMX_VIDEO_CodingMPEG4:      printf("OMX_VIDEO_CodingMPEG4\n"); break;
    case OMX_VIDEO_CodingWMV:        printf("OMX_VIDEO_CodingWMV\n"); break;
    case OMX_VIDEO_CodingRV:         printf("OMX_VIDEO_CodingRV\n"); break;
    case OMX_VIDEO_CodingAVC:        printf("OMX_VIDEO_CodingAVC\n"); break;
    case OMX_VIDEO_CodingMJPEG:      printf("OMX_VIDEO_CodingMJPEG\n"); break;
    case OMX_VIDEO_CodingMax:        printf("OMX_VIDEO_CodingMax (not a video format)\n"); break;
    default: printf("Format not accounted for"); break;
    }
  switch(video.eColorFormat)
    {
    case OMX_COLOR_FormatUnused:                 printf("OMX_COLOR_FormatUnused\n"); break;
    case OMX_COLOR_FormatMonochrome:             printf("OMX_COLOR_FormatMonochrome\n"); break;
    case OMX_COLOR_Format8bitRGB332:             printf("OMX_COLOR_Format8bitRGB332\n"); break;
    case OMX_COLOR_Format12bitRGB444:            printf("OMX_COLOR_Format12bitRGB444\n"); break;
    case OMX_COLOR_Format16bitARGB4444:          printf("OMX_COLOR_Format16bitARGB4444\n"); break;
    case OMX_COLOR_Format16bitARGB1555:          printf("OMX_COLOR_Format16bitARGB1555\n"); break;
    case OMX_COLOR_Format16bitRGB565:            printf("OMX_COLOR_Format16bitRGB565\n"); break;
    case OMX_COLOR_Format16bitBGR565:            printf("OMX_COLOR_Format16bitBGR565\n"); break;
    case OMX_COLOR_Format18bitRGB666:            printf("OMX_COLOR_Format18bitRGB666\n"); break;
    case OMX_COLOR_Format18bitARGB1665:          printf("OMX_COLOR_Format18bitARGB1665\n"); break;
    case OMX_COLOR_Format19bitARGB1666:          printf("OMX_COLOR_Format19bitARGB1666\n"); break;
    case OMX_COLOR_Format24bitRGB888:            printf("OMX_COLOR_Format24bitRGB888\n"); break;
    case OMX_COLOR_Format24bitBGR888:            printf("OMX_COLOR_Format24bitBGR888\n"); break;
    case OMX_COLOR_Format24bitARGB1887:          printf("OMX_COLOR_Format24bitARGB1887\n"); break;
    case OMX_COLOR_Format25bitARGB1888:          printf("OMX_COLOR_Format25bitARGB1888\n"); break;
    case OMX_COLOR_Format32bitBGRA8888:          printf("OMX_COLOR_Format32bitBGRA8888\n"); break;
    case OMX_COLOR_Format32bitARGB8888:          printf("OMX_COLOR_Format32bitARGB8888\n"); break;
    case OMX_COLOR_FormatYUV411Planar:           printf("OMX_COLOR_FormatYUV411Planar\n"); break;
    case OMX_COLOR_FormatYUV411PackedPlanar:     printf("OMX_COLOR_FormatYUV411PackedPlanar\n"); break;
    case OMX_COLOR_FormatYUV420Planar:           printf("OMX_COLOR_FormatYUV420Planar\n"); break;
    case OMX_COLOR_FormatYUV420PackedPlanar:     printf("OMX_COLOR_FormatYUV420PackedPlanar\n"); break;
    case OMX_COLOR_FormatYUV420SemiPlanar:       printf("OMX_COLOR_FormatYUV420SemiPlanar\n"); break;
    case OMX_COLOR_FormatYUV422Planar:           printf("OMX_COLOR_FormatYUV422Planar\n"); break;
    case OMX_COLOR_FormatYUV422PackedPlanar:     printf("OMX_COLOR_FormatYUV422PackedPlanar\n"); break;
    case OMX_COLOR_FormatYUV422SemiPlanar:       printf("OMX_COLOR_FormatYUV422SemiPlanar\n"); break;
    case OMX_COLOR_FormatYCbYCr:                 printf("OMX_COLOR_FormatYCbYCr\n"); break;
    case OMX_COLOR_FormatYCrYCb:                 printf("OMX_COLOR_FormatYCrYCb\n"); break;
    case OMX_COLOR_FormatCbYCrY:                 printf("OMX_COLOR_FormatCbYCrY\n"); break;
    case OMX_COLOR_FormatCrYCbY:                 printf("OMX_COLOR_FormatCrYCbY\n"); break;
    case OMX_COLOR_FormatYUV444Interleaved:      printf("OMX_COLOR_FormatYUV444Interleaved\n"); break;
    case OMX_COLOR_FormatRawBayer8bit:           printf("OMX_COLOR_FormatRawBayer8bit\n"); break;
    case OMX_COLOR_FormatRawBayer10bit:          printf("OMX_COLOR_FormatRawBayer10bit\n"); break;
    case OMX_COLOR_FormatRawBayer8bitcompressed: printf("OMX_COLOR_FormatRawBayer8bitcompressed\n"); break;
    case OMX_COLOR_FormatL2:                     printf("OMX_COLOR_FormatL2\n"); break;
    case OMX_COLOR_FormatL4:                     printf("OMX_COLOR_FormatL4\n"); break;
    case OMX_COLOR_FormatL8:                     printf("OMX_COLOR_FormatL8\n"); break;
    case OMX_COLOR_FormatL16:                    printf("OMX_COLOR_FormatL16\n"); break;
    case OMX_COLOR_FormatL24:                    printf("OMX_COLOR_FormatL24\n"); break;
    case OMX_COLOR_FormatL32:                    printf("OMX_COLOR_FormatL32\n"); break;
    case OMX_COLOR_FormatYUV420PackedSemiPlanar: printf("OMX_COLOR_FormatYUV420PackedSemiPlanar\n"); break;
    case OMX_COLOR_FormatYUV422PackedSemiPlanar: printf("OMX_COLOR_FormatYUV422PackedSemiPlanar\n"); break;
    case OMX_COLOR_Format18BitBGR666:            printf("OMX_COLOR_Format18BitBGR666\n"); break;
    case OMX_COLOR_Format24BitARGB6666:          printf("OMX_COLOR_Format24BitARGB6666\n"); break;
    case OMX_COLOR_Format24BitABGR6666:          printf("OMX_COLOR_Format24BitABGR6666\n"); break;
    case OMX_COLOR_FormatMax:                    printf("OMX_COLOR_FormatMax (not a format)\n"); break;
    default: printf("Format not accounted for\n"); break;
    }
  printf("pNativeWindow = %d\n", (int)video.pNativeWindow);
}

void print_OMX_IMAGE_PORTDEFINITIONTYPE(OMX_IMAGE_PORTDEFINITIONTYPE image)
{
  printf("Not implemented yet\n");
}

void print_OMX_OTHER_PORTDEFINITIONTYPE(OMX_OTHER_PORTDEFINITIONTYPE other)
{
  printf("Not implemented yet\n");
}

//prints port params
void print_OMX_PARAM_PORTDEFINITIONTYPE(OMX_PARAM_PORTDEFINITIONTYPE params)
{
  printf("-----------------------\n");
  printf("nSize = %d\n", (int)params.nSize);
  printf("nVersion.nVersion.nVersionMajor = %d\n", (int)params.nVersion.s.nVersionMajor);
  printf("                 .nVersionMinor = %d\n", (int)params.nVersion.s.nVersionMinor);
  printf("                 .nReviseion = %d\n",    (int)params.nVersion.s.nRevision);
  //printf("                 .nStep = &d\n",         (int)params.nVersion.s.nStep);
  printf("nPortIndex = %d\n", params.nPortIndex);
  switch(params.eDir)
    {
    case OMX_DirInput: printf("eDir = OMX_DirInput (Input port)\n"); break;
    case OMX_DirOutput: printf("eDir = OMX_DirOutput (Output port)\n"); break;
    case OMX_DirMax: printf("eDir = OMX_DirMax (?)\n"); break;
    default: printf("Possible Error: unable to determin direction of port\n");
    } 
  printf("nBufferCountActual = %d\n", (int)params.nBufferCountActual);
  printf("nBufferCountMin = %d\n", (int)params.nBufferCountMin);
  printf("nBufferSize = %d\n", (int)params.nBufferSize);
  printf("bEnabled = "); params.bEnabled ? printf("true\n") : printf("false\n");
  printf("bPopulated = "); params.bPopulated ? printf("true\n") : printf("false\n");
  switch(params.eDomain)
    {
    case OMX_PortDomainAudio:
      printf("eDomain = OMX_PortDomainAudio (Audio Port)\n");
      print_OMX_AUDIO_PORTDEFINITIONTYPE(params.format.audio);
      break;
    case OMX_PortDomainVideo:
      printf("eDomain = OMX_PortDomainVideo (Video Port)\n");
      print_OMX_VIDEO_PORTDEFINITIONTYPE(params.format.video);
      break;
    case OMX_PortDomainImage:
      printf("eDomain = OMX_PortDomainImage (Image Port)\n");
      print_OMX_IMAGE_PORTDEFINITIONTYPE(params.format.image);
      break;
    case OMX_PortDomainOther:
      printf("eDomain = OMX_PortDomainOther (Other Port)\n");
      print_OMX_OTHER_PORTDEFINITIONTYPE(params.format.other);
      break;
    default:
      printf("eDomain = is another type not accounted for (?)\n");
      break;
    }


  printf("bBuffersContiguous = "); params.bBuffersContiguous ? printf("true\n") : printf("false\n");
  printf("nBufferAlignment = %d\n", (int)params.nBufferAlignment);
  
  printf("-----------------------\n");
}

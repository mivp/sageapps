#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "DeckLinkAPI.h"
#include "decklinkcapture.h"
#include "ring.h"

// headers for SAGE
#include "libsage.h"

// SAGE UI API
#include "suil.h"
suil uiLib;

// SAGE
sail *sageInf; // sail object
int  useSAGE = 0;
int  useSAM  = 0;
int  sageW, sageH;
double sageFPS;
int useSBS = 0;
int stride;
unsigned char *sbsBuffer = NULL;
unsigned char *rgbImageData = NULL;
int image_width, image_height;

// SAM
Ring_Buffer *audio_ring;
#define AUDIO_TEMP 16384
int audio_temp[AUDIO_TEMP]; //32bit
sam::StreamingAudioClient* m_sac = NULL;
int physicalInputs = 0;
char *samIP;

// if achieving less than threshold frame rate
//     then drop frames
#define DROP_THRESHOLD 0.95

// Decklink
pthread_mutex_t			sleepMutex;
pthread_cond_t			sleepCond;
int				videoOutputFile = -1;
int				audioOutputFile = -1;
int				card = 0;
IDeckLink 			*deckLink;
IDeckLinkInput			*deckLinkInput;
IDeckLinkDisplayModeIterator	*displayModeIterator;

static BMDTimecodeFormat	g_timecodeFormat = 0;
static int			g_videoModeIndex = -1;
static int			g_audioChannels = 2;
static int			g_audioSampleDepth = 32;
const char *			g_videoOutputFile = NULL;
const char *			g_audioOutputFile = NULL;
static int			g_maxFrames = -1;
static unsigned long 		frameCount = 0;
BMDTimeValue frameRateDuration, frameRateScale;

//typedef unsigned char BYTE;
void YUV422UYVY_ToRGB24(unsigned char* pbYUV, unsigned char* pbRGB, int yuv_len)
{
	// yuv_len: number of bytes
	int B_Cb128,R_Cr128,G_CrCb128;
    int Y0,U,Y1,V;
    int R,G,B;

	unsigned char *yuv_index;
	unsigned char *rgb_index;

	yuv_index = pbYUV;
	rgb_index = pbRGB;

    for (int i = 0, j = 0; i < yuv_len; )
    {
		U  = (int)((float)yuv_index[i++]-128.0f);
		Y0 = (int)(1.164f * ((float)pbYUV[i++]-16.0f));
		V  = (int)((float)yuv_index[i++]-128.0f);
		Y1 = (int)(1.164f * ((float)yuv_index[i++]-16.0f));

		R_Cr128   =  (int)(1.596f*V);
		G_CrCb128 = (int)(-0.813f*V - 0.391f*U);
		B_Cb128   =  (int)(2.018f*U);

		R= Y0 + R_Cr128;
		G = Y0 + G_CrCb128;
		B = Y0 + B_Cb128;

		rgb_index[j++] = max(0, min(R, 255));
		rgb_index[j++] = max(0, min(G, 255));
		rgb_index[j++] = max(0, min(B, 255));

		R= Y1 + R_Cr128;
		G = Y1 + G_CrCb128;
		B = Y1 + B_Cb128;

		rgb_index[j++] = max(0, min(R, 255));
		rgb_index[j++] = max(0, min(G, 255));
		rgb_index[j++] = max(0, min(B, 255));
	}
}


void* readThread(void *args)
{
   suil *uiLib = (suil *)args;

   while(1) {
      sageMessage msg;
      msg.init(READ_BUF_SIZE);
      uiLib->rcvMessageBlk(msg);
      if ( msg.getCode() == (APP_INFO_RETURN) ) {
		//sage::printLog("UI Message> %d --- %s", msg.getCode(), (char *)msg.getData()); 
		// format:   decklinkcapture 13 1972 5172 390 2190 1045 0 0 0 0 none 1280 720 1
		char appname[256], cmd[256];
		int ret;
		int appID, left, right, bottom, top, sailID, zValue;
		ret = sscanf((char*)msg.getData(), "%s %d %d %d %d %d %d %d",
			appname, &appID, &left, &right, &bottom, &top, &sailID, &zValue);
		if (ret == 8) {
			if (sageInf->getWinID() == appID) {
				int x,y,w,h;
				x = left;
				y = top;
				w = right-left;
				h = top-bottom;
				if (useSAM && m_sac) {
					int ret;
					sage::printLog("\t ME ME ME: %d x %d - %d x %d - %d", x,y,w,h, zValue);
					ret = m_sac->setPosition(x,y,w,h,zValue);
				}
			}
		}
	}
   }

   return NULL;
}


DeckLinkCaptureDelegate::DeckLinkCaptureDelegate() : m_refCount(0)
{
	pthread_mutex_init(&m_mutex, NULL);
}

DeckLinkCaptureDelegate::~DeckLinkCaptureDelegate()
{
	pthread_mutex_destroy(&m_mutex);
}

ULONG DeckLinkCaptureDelegate::AddRef(void)
{
	pthread_mutex_lock(&m_mutex);
		m_refCount++;
	pthread_mutex_unlock(&m_mutex);

	return (ULONG)m_refCount;
}

ULONG DeckLinkCaptureDelegate::Release(void)
{
	pthread_mutex_lock(&m_mutex);
		m_refCount--;
	pthread_mutex_unlock(&m_mutex);

	if (m_refCount == 0)
	{
		delete this;
		return 0;
	}

	return (ULONG)m_refCount;
}

void decklinkQuit(sail *si)
{
	pthread_cond_signal(&sleepCond);
	if (m_sac) delete m_sac;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame)
{
	IDeckLinkVideoFrame*	                rightEyeFrame = NULL;
	IDeckLinkVideoFrame3DExtensions*        threeDExtensions = NULL;
	void*					frameBytes;
	void*					audioFrameBytes;
	static int warmup = (int)sageFPS; // 1 sec of frames
	static BMDTimeValue startoftime;
	static int skipf = 0;
	static double lastframetime = 0.0;

	// Handle Video Frame
	if(videoFrame)
	{	

		if (frameCount==0) {
			BMDTimeValue hframedur;
			HRESULT h = videoFrame->GetHardwareReferenceTimestamp(frameRateScale, &startoftime, &hframedur);
			lastframetime = sage::getTime();
		}

		if (videoFrame->GetFlags() & bmdFrameHasNoInputSource)
		{
			fprintf(stderr, "Frame received (#%lu) - No input signal detected\n", frameCount);
		}
		else
		{
			if (useSAGE) {
				BMDTimeScale htimeScale = frameRateScale;
				BMDTimeValue hframeTime;
				BMDTimeValue hframeDuration;
				HRESULT h = videoFrame->GetHardwareReferenceTimestamp(htimeScale, &hframeTime, &hframeDuration);
				if (h == S_OK) {
					double frametime = (double)(hframeTime-startoftime) / (double)hframeDuration;
					double instantfps = 1000000.0/(sage::getTime()-lastframetime);
					if ( ((int)frametime) > frameCount) { skipf = 1; frameCount++; }
					if ( instantfps <= (sageFPS*DROP_THRESHOLD) ) { skipf = 1; frameCount++; }   // if lower than 95% of target FPS
					//if (skipf)
						//sage::printLog("HARD Time: %10ld %10ld %10ld: %f %d %f --", hframeTime, hframeDuration, htimeScale, frametime, frameCount, instantfps );
					//else
						//sage::printLog("HARD Time: %10ld %10ld %10ld: %f %d %f", hframeTime, hframeDuration, htimeScale, frametime, frameCount, instantfps );
					lastframetime = sage::getTime();
				} else {
					BMDTimeValue frameTime, frameDuration;
					videoFrame->GetStreamTime(&frameTime, &frameDuration, frameRateScale);

					sage::printLog("SOFT Time: %10ld %10ld %10ld", frameTime, frameDuration, frameRateScale);
				}

				//double actualFPS = (double)frameRateScale / (double)frameRateDuration;

				if (!skipf) {
					// Get the pixels from the capture
					videoFrame->GetBytes(&frameBytes);

					// Pass the pixels to SAGE
					int localskip = 0;
					if (sageFPS >= 50.0) {                // if 50p or 60p rate
						if ( (frameCount%2) == 0 ) {  // skip every other frame
							localskip = 1;
						}
					}
					if (!localskip)
					{
						// process video frame for SAGE
						if(useSBS)
						{
							//convert YUV -> RGB
							YUV422UYVY_ToRGB24((unsigned char*)frameBytes, rgbImageData, image_width*image_height*2);

							//stereo
							sbsBuffer = nextBuffer(sageInf);
							int bpp = 6; // 6 bytes/pixel
							for (int i=0;i<image_height;i++) {
                        		for (int j=0;j<stride;j++) {
		                            sbsBuffer[i*stride*bpp + j*bpp + 3] = rgbImageData[i*image_width*3 + j*3 + 0]; // Red   left
		                            sbsBuffer[i*stride*bpp + j*bpp + 4] = rgbImageData[i*image_width*3 + j*3 + 1]; // Green left
		                            sbsBuffer[i*stride*bpp + j*bpp + 5] = rgbImageData[i*image_width*3 + j*3 + 2]; // Blue  left
		                            sbsBuffer[i*stride*bpp + j*bpp + 0] = rgbImageData[i*image_width*3 + stride*3 + j*3 + 0]; // Red   right
		                            sbsBuffer[i*stride*bpp + j*bpp + 1] = rgbImageData[i*image_width*3 + stride*3 + j*3 + 1]; // Green right
		                            sbsBuffer[i*stride*bpp + j*bpp + 2] = rgbImageData[i*image_width*3 + stride*3 + j*3 + 2]; // Blue  right
		                        }
		                    }
							
							// send data to SAGE
							sageInf->swapBuffer();
							//swapWithBuffer(sageInf, sbsBuffer); 
						}
						else
						{
							// send data to SAGE
							swapWithBuffer(sageInf, (unsigned char *)frameBytes);
						}
					}				

					processMessages(sageInf, NULL,decklinkQuit,NULL);
				} else {
					sage::printLogNL("#");
				}

			}
			else if (videoOutputFile != -1)
			{
				videoFrame->GetBytes(&frameBytes);
				write(videoOutputFile, frameBytes, videoFrame->GetRowBytes() * videoFrame->GetHeight());
				
				if (rightEyeFrame)
				{
					rightEyeFrame->GetBytes(&frameBytes);
					write(videoOutputFile, frameBytes, videoFrame->GetRowBytes() * videoFrame->GetHeight());
				}
			}
		}
		frameCount++;

		if (g_maxFrames > 0 && frameCount >= g_maxFrames)
		{
			pthread_cond_signal(&sleepCond);
		}
	}

	// Handle Audio Frame
	if (audioFrame)
	{
		if (useSAM && ! physicalInputs ) {
			int gotFrames = audioFrame->GetSampleFrameCount();
			audioFrame->GetBytes(&audioFrameBytes);
			//fprintf(stderr, "Audio Frame received: %d bytes - Samples %d\n",
				//gotFrames * g_audioChannels * (g_audioSampleDepth / 8), gotFrames );
			unsigned int len =  gotFrames * g_audioChannels * (g_audioSampleDepth / 8);
			unsigned int putin =  len;
			audio_ring->Buffer_Data(audioFrameBytes, putin);
			//fprintf(stderr, "Added: %6u / %6u -- Buffered: %6u -- Free %6u\n", len, putin, audio_ring->Buffered_Bytes(), audio_ring->Free_Space());
		}
		else {
			if (audioOutputFile != -1)
			{
				audioFrame->GetBytes(&audioFrameBytes);
				write(audioOutputFile, audioFrameBytes, audioFrame->GetSampleFrameCount() * g_audioChannels * (g_audioSampleDepth / 8));
			}
		}
	}


    //frameCount++;
	skipf = 0;

    return S_OK;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags)
{
    fprintf(stderr, "VideoInputFormatChanged VideoInputFormatChanged VideoInputFormatChanged\n");
    fprintf(stderr, "VideoInputFormatChanged VideoInputFormatChanged VideoInputFormatChanged\n");
    fprintf(stderr, "VideoInputFormatChanged VideoInputFormatChanged VideoInputFormatChanged\n");
    return S_OK;
}

int usage(int status)
{
	HRESULT result;
	IDeckLinkDisplayMode *displayMode;
	int displayModeCount = 0;

	fprintf(stderr, 
		"Usage: declinkcapture -d <card id> -m <mode id> [OPTIONS]\n"
		"\n"
		"    -d <card id>:\n");

        IDeckLinkIterator *di = CreateDeckLinkIteratorInstance();
	IDeckLink *dl;
	int dnum = 0;
        while (di->Next(&dl) == S_OK)
        {
		const char *deviceNameString = NULL;
		int  result = dl->GetModelName(&deviceNameString);
		if (result == S_OK)
		{
			fprintf(stderr, "\t device %d [%s]\n", dnum, deviceNameString);
		}
		dl->Release();
		dnum++;
	}


	fprintf(stderr, 
		"    -m <mode id>:\n"
	);

    while (displayModeIterator->Next(&displayMode) == S_OK)
    {
        char *displayModeString = NULL;

        result = displayMode->GetName((const char **) &displayModeString);
        if (result == S_OK)
        {
		BMDTimeValue iframeRateDuration, iframeRateScale;
		displayMode->GetFrameRate(&iframeRateDuration, &iframeRateScale);

		fprintf(stderr, "        %2d:  %-20s \t %li x %li \t %g FPS\n", 
			displayModeCount, displayModeString, displayMode->GetWidth(), displayMode->GetHeight(), (double)iframeRateScale / (double)iframeRateDuration);
			
		free(displayModeString);
		displayModeCount++;
        }

        // Release the IDeckLinkDisplayMode object to prevent a leak
        displayMode->Release();
    }

	fprintf(stderr, 
		"    -i <video input number>\n" 
		"    -p <pixelformat>\n" 
		"         0:  8 bit YUV (4:2:2) (default)\n"
		"         1:  10 bit YUV (4:2:2)\n"
		"         2:  10 bit RGB (4:4:4)\n"
		"    -t <format>          Print timecode\n"
		"     rp188:  RP 188\n"
		"      vitc:  VITC\n"
		"    serial:  Serial Timecode\n"
		"    -f <filename>        Filename raw video will be written to\n"
		"    -a <filename>        Filename raw audio will be written to\n"
		"    -c <channels>        Audio Channels (2, 8 or 16 - default is 2)\n"
		"    -s <depth>           Audio Sample Depth (16 or 32 - default is 32)\n"
		"    -n <frames>          Number of frames to capture (default is unlimited)\n"
		"    -3                   Capture Stereoscopic 3D (Requires 3D Hardware support)\n"
		"    -u <SAM IP>          stream using SAM (audio), 127.0.0.1 by default \n"
		"    -j                   physical inputs for SAM inputs (audio)\n"
		"    -v                   stream using SAGE (video)\n"
		"    -y                   apply deinterlacing filter\n"
		"\n"
		"Capture video and/or audio to a file. Raw video and/or audio can be viewed with mplayer eg:\n"
		"\n"
		"    Capture -m2 -n 50 -f video.raw -a audio.raw\n"
		"    mplayer video.raw -demuxer rawvideo -rawvideo pal:uyvy -audiofile audio.raw -audio-demuxer 20 -rawaudio rate=48000\n"
	);

	exit(status);
}

bool handle_sac_audio_callback(unsigned int numChannels, unsigned int nframes, float** out, void*obj)
{
	unsigned int alen = nframes  * g_audioChannels * (g_audioSampleDepth / 8);
	unsigned int datalen = nframes  * g_audioChannels * 4;

	for (unsigned int k = 0; k < nframes*numChannels; k++) {
		audio_temp[k] = 0;
	}
	for (unsigned int ch = 0; ch < 1; ch++)
		for (unsigned int n = 0; n < nframes; n++)
			out[ch][n] = 0.0f;

	//fprintf(stderr, "Callback: asking %d frames (%d bytes)\n", nframes, datalen);

#if 1
	unsigned int got = nframes  * g_audioChannels * (g_audioSampleDepth / 8);
	audio_ring->Get_Data(audio_temp, got);
	int numsamples = got / (g_audioChannels * (g_audioSampleDepth / 8));
	if (got == 0) {
		//fprintf(stderr, "No data queue\n");
		fprintf(stderr, "#");
	}
	if ((got > 0) && (got != alen)) {
		fprintf(stderr, ".");
		//fprintf(stderr, "seconds: asked %u got %u -- Buffered: %6u -- Free %6u\n",
			//datalen, got, audio_ring->Buffered_Bytes(), audio_ring->Free_Space());
	}
	if (got == alen) {
		//fprintf(stderr, "Good: asked %u got %u -- Buffered: %6u -- Free %6u\n",
			//datalen, got, audio_ring->Buffered_Bytes(), audio_ring->Free_Space());
	}

	float *ptr = *out;
	int value; // for 32bit capture
	float fvalue;
	for (int k = 0; k < numsamples; k++) {
		for (int ch = 0; ch < g_audioChannels; ch++) {
			value = audio_temp[g_audioChannels*k+ch];
			fvalue = value / (float)(RAND_MAX); // for 32bit sound data
			out[ch][k] = fvalue;
		}
	}
        


	//write(audioOutputFile, ptr+numsamples, numsamples* 4);
#else
    for (unsigned int ch = 0; ch < 2; ch++)
       for (unsigned int n = 0; n < nframes; n++)
            out[ch][n] = rand() / (float)RAND_MAX;
#endif

    return true;
}


void myQuit(int sig)
{
	fprintf(stderr, "Caught signal [%d]\n", sig);

	// Quit SAM
	if (m_sac) delete m_sac;

	if (sageInf) deleteSAIL(sageInf);
}

char *connectionName(int64_t vport)
{
    char *name;
    switch (vport) {
	case bmdVideoConnectionSDI:
	name = strdup("SDI");
	break;
	case bmdVideoConnectionHDMI:
	name = strdup("HDMI");
	break;
	case bmdVideoConnectionComponent:
	name = strdup("Component");
	break;
	case bmdVideoConnectionComposite:
	name = strdup("Composite");
	break;
	case bmdVideoConnectionSVideo:
	name = strdup("SVideo");
	break;
	case bmdVideoConnectionOpticalSDI:
	name = strdup("OpticalSDI");
	break;
	default:
	name = strdup("UNKNOWN");
	break;
    }
}


int main(int argc, char *argv[])
{
    signal(SIGABRT,myQuit);//If program aborts go to assigned function
    signal(SIGTERM,myQuit);//If program terminates go to assigned function
    signal(SIGINT, myQuit);//If program terminates go to assigned function

	IDeckLinkIterator		*deckLinkIterator = CreateDeckLinkIteratorInstance();
        IDeckLinkAttributes*                            deckLinkAttributes = NULL;
	DeckLinkCaptureDelegate 	*delegate;
	IDeckLinkDisplayMode		*displayMode;
	BMDVideoInputFlags		inputFlags = 0;
	BMDDisplayMode			selectedDisplayMode = bmdModeNTSC;
	BMDPixelFormat			pixelFormat = bmdFormat8BitYUV;
	int				displayModeCount = 0;
	int				exitStatus = 1;
	int				ch;
	bool 				foundDisplayMode = false;
	HRESULT				result;
	int				dnum = 0;
	IDeckLink         		*tempLink = NULL;
	int				found = 0;
	bool				supported = 0;
        int64_t                                         ports;
        int                                                     itemCount;
        int                                                     vinput = 0;
        int64_t                                                     vport = 0;
	        IDeckLinkConfiguration       *deckLinkConfiguration = NULL;
    bool flickerremoval = true;
    //bool pnotpsf = false;
    bool pnotpsf = true;

	// Default IP address for SAM
	samIP = strdup("127.0.0.1");

	pthread_mutex_init(&sleepMutex, NULL);
	pthread_cond_init(&sleepCond, NULL);
	
	// Parse command line options
	while ((ch = getopt(argc, argv, "?h3c:bd:s:f:a:m:n:p:t:u::vi:j")) != -1) 
	{
		switch (ch) 
		{
			case 'i':
				vinput = atoi(optarg);
				break;
			case 'u':
				useSAM  = 1;
				sage::printLog("Set useSAM to 1: [%s]", optarg);
				if (optarg)
					samIP = strdup(optarg);
				break;
			case 'v':
				useSAGE = 1;
				break;
			case 'j':
				physicalInputs = 1;
				break;
			case 'b':
				useSBS = 1;
				break;
			case 'd':
				card = atoi(optarg);
				break;
			case 'm':
				g_videoModeIndex = atoi(optarg);
				break;
			case 'c':
				g_audioChannels = atoi(optarg);
				if (g_audioChannels != 2 &&
				    g_audioChannels != 8 &&
					g_audioChannels != 16)
				{
					fprintf(stderr, "Invalid argument: Audio Channels must be either 2, 8 or 16\n");
					goto bail;
				}
				break;
			case 's':
				g_audioSampleDepth = atoi(optarg);
				if (g_audioSampleDepth != 16 && g_audioSampleDepth != 32)
				{
					fprintf(stderr, "Invalid argument: Audio Sample Depth must be either 16 bits or 32 bits\n");
					goto bail;
				}
				break;
			case 'f':
				g_videoOutputFile = optarg;
				break;
			case 'a':
				g_audioOutputFile = optarg;
				break;
			case 'n':
				g_maxFrames = atoi(optarg);
				break;
			case '3':
				inputFlags |= bmdVideoInputDualStream3D;
				break;
			case 'p':
				switch(atoi(optarg))
				{
					case 0: pixelFormat = bmdFormat8BitYUV; break;
					case 1: pixelFormat = bmdFormat10BitYUV; break;
					case 2: pixelFormat = bmdFormat10BitRGB; break;
					default:
						fprintf(stderr, "Invalid argument: Pixel format %d is not valid", atoi(optarg));
						goto bail;
				}
				break;
			case 't':
				if (!strcmp(optarg, "rp188"))
					g_timecodeFormat = bmdTimecodeRP188Any;
    			else if (!strcmp(optarg, "vitc"))
					g_timecodeFormat = bmdTimecodeVITC;
    			else if (!strcmp(optarg, "serial"))
					g_timecodeFormat = bmdTimecodeSerial;
				else
				{
					fprintf(stderr, "Invalid argument: Timecode format \"%s\" is invalid\n", optarg);
					goto bail;
				}
				break;
			case '?':
			case 'h':
				usage(0);
		}
	}

	if (useSAGE && (pixelFormat != bmdFormat8BitYUV)) {
		sage::printLog("SAGE works only with pixelFormat 1 (bmdFormat8BitYUV)");
		pixelFormat = bmdFormat8BitYUV;
	}

	if (!deckLinkIterator)
	{
		fprintf(stderr, "This application requires the DeckLink drivers installed.\n");
		goto bail;
	}
	
	/* Connect to the first DeckLink instance */
	while (deckLinkIterator->Next(&tempLink) == S_OK)
	{
		if (card != dnum) {
			dnum++;
			// Release the IDeckLink instance when we've finished with it to prevent leaks
			tempLink->Release();
			continue;
		}
		else {
			deckLink = tempLink;
			found = 1;
		}
		dnum++;
	}

	if (! found ) {
		fprintf(stderr, "No DeckLink PCI cards found.\n");
		goto bail;
	}
	if (deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput) != S_OK)
		goto bail;

	{
        // Query the DeckLink for its attributes interface
        result = deckLink->QueryInterface(IID_IDeckLinkAttributes, (void**)&deckLinkAttributes);
        if (result != S_OK)
	  	{
	    	fprintf(stderr, "Could not obtain the IDeckLinkAttributes interface - result = %08x\n", result);
	  	}
	}

    result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &supported);
	if (result == S_OK)
	{
		fprintf(stderr, " %-40s %s\n", "Input mode detection supported ?", (supported == true) ? "Yes" : "No");
	}
	else
	{
		fprintf(stderr, "Could not query the input mode detection attribute- result = %08x\n", result);
	}

    fprintf(stderr, "Supported video input connections (-i [input #]:\n  ");
    itemCount = 0;
    result = deckLinkAttributes->GetInt(BMDDeckLinkVideoInputConnections, &ports);
    if (result == S_OK)
    {
        if (ports & bmdVideoConnectionSDI)
        {
                fprintf(stderr, "%d: SDI, ", bmdVideoConnectionSDI);
                itemCount++;
        }

        if (ports & bmdVideoConnectionHDMI)
        {
                fprintf(stderr, "%d: HDMI, ", bmdVideoConnectionHDMI);
                itemCount++;
        }

        if (ports & bmdVideoConnectionOpticalSDI)
        {
                fprintf(stderr, "%d: Optical SDI, ", bmdVideoConnectionOpticalSDI);
                itemCount++;
        }

        if (ports & bmdVideoConnectionComponent)
        {
                fprintf(stderr, "%d: Component, ", bmdVideoConnectionComponent);
                itemCount++;
        }

        if (ports & bmdVideoConnectionComposite)
        {
                fprintf(stderr, "%d: Composite, ", bmdVideoConnectionComposite);
                itemCount++;
        }

        if (ports & bmdVideoConnectionSVideo)
        {
                fprintf(stderr, "%d: S-Video, ", bmdVideoConnectionSVideo);
                itemCount++;
        }
    }
	fprintf(stderr, "\n");


	delegate = new DeckLinkCaptureDelegate();
	deckLinkInput->SetCallback(delegate);
   
	// Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output
	result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the video output display mode iterator - result = %08x\n", result);
		goto bail;
	}
	

	if (g_videoModeIndex < 0)
	{
		fprintf(stderr, "No video mode specified\n");
		usage(0);
	}

	if (g_videoOutputFile != NULL)
	{
		videoOutputFile = open(g_videoOutputFile, O_WRONLY|O_CREAT|O_TRUNC, 0664);
		if (videoOutputFile < 0)
		{
			fprintf(stderr, "Could not open video output file \"%s\"\n", g_videoOutputFile);
			goto bail;
		}
	}
	if (g_audioOutputFile != NULL)
	{
		audioOutputFile = open(g_audioOutputFile, O_WRONLY|O_CREAT|O_TRUNC, 0664);
		if (audioOutputFile < 0)
		{
			fprintf(stderr, "Could not open audio output file \"%s\"\n", g_audioOutputFile);
			goto bail;
		}
	}
	
	while (displayModeIterator->Next(&displayMode) == S_OK)
	{
		if (g_videoModeIndex == displayModeCount)
		{
			BMDDisplayModeSupport result;
			const char *displayModeName;
			
			foundDisplayMode = true;
			displayMode->GetName(&displayModeName);
			selectedDisplayMode = displayMode->GetDisplayMode();
			
			deckLinkInput->DoesSupportVideoMode(selectedDisplayMode, pixelFormat, bmdVideoInputFlagDefault, &result, NULL);

			if (result == bmdDisplayModeNotSupported)
			{
				fprintf(stderr, "The display mode %s is not supported with the selected pixel format\n", displayModeName);
				goto bail;
			}

			if (inputFlags & bmdVideoInputDualStream3D)
			{
				if (!(displayMode->GetFlags() & bmdDisplayModeSupports3D))
				{
					fprintf(stderr, "The display mode %s is not supported with 3D\n", displayModeName);
					goto bail;
				}
			}
			fprintf(stderr, "Selecting mode: %s\n", displayModeName);
			
			break;
		}
		displayModeCount++;
		displayMode->Release();
	}

	if (!foundDisplayMode)
	{
		fprintf(stderr, "Invalid mode %d specified\n", g_videoModeIndex);
		goto bail;
	}

    {
	    // Query the DeckLink for its configuration interface
	    result = deckLinkInput->QueryInterface(IID_IDeckLinkConfiguration, (void**)&deckLinkConfiguration);
	    if (result != S_OK)
	    {
			fprintf(stderr, "Could not obtain the IDeckLinkConfiguration interface: %08x\n", result);
	    }
    }

    BMDVideoConnection conn;
    switch (vinput) {
    case 0:
      conn = bmdVideoConnectionSDI;
      break;
    case 1:
      conn = bmdVideoConnectionHDMI;
      break;
    case 2:
      conn = bmdVideoConnectionComponent;
      break;
    case 3:
      conn = bmdVideoConnectionComposite;
      break;
    case 4:
      conn = bmdVideoConnectionSVideo;
      break;
    case 5:
      conn = bmdVideoConnectionOpticalSDI;
      break;
    default:
      break;
    }
   conn = vinput;
    // Set the input desired
    result = deckLinkConfiguration->SetInt(bmdDeckLinkConfigVideoInputConnection, conn);
    if(result != S_OK) {
	fprintf(stderr, "Cannot set the input to [%d]\n", conn);
	goto bail;
    }

    // check input
    result = deckLinkConfiguration->GetInt(bmdDeckLinkConfigVideoInputConnection, &vport);
    if (vport == bmdVideoConnectionSDI)
	      fprintf(stderr, "Before Input configured for SDI\n");
    if (vport == bmdVideoConnectionHDMI)
	      fprintf(stderr, "Before Input configured for HDMI\n");


    if (deckLinkConfiguration->SetFlag(bmdDeckLinkConfigFieldFlickerRemoval, flickerremoval) == S_OK) {
      fprintf(stderr, "Flicker removal set : %d\n", flickerremoval);
    }
    else {
      fprintf(stderr, "Flicker removal NOT set\n");
    }

    if (deckLinkConfiguration->SetFlag(bmdDeckLinkConfigUse1080pNotPsF, pnotpsf) == S_OK) {
      fprintf(stderr, "bmdDeckLinkConfigUse1080pNotPsF: %d\n", pnotpsf);
    }
    else {
      fprintf(stderr, "bmdDeckLinkConfigUse1080pNotPsF NOT set\n");
    }

    //if (deckLinkConfiguration->SetFlag(bmdDeckLinkConfigVideoInputConnection, conn) == S_OK) {
      //fprintf(stderr, "Input set to: %d\n", vinput);
    //}

    result = deckLinkConfiguration->GetInt(bmdDeckLinkConfigVideoInputConnection, &vport);
    if (vport == bmdVideoConnectionSDI)
	      fprintf(stderr, "After Input configured for SDI\n");
    if (vport == bmdVideoConnectionHDMI)
	      fprintf(stderr, "After Input configured for HDMI\n");



    result = deckLinkInput->EnableVideoInput(selectedDisplayMode, pixelFormat, inputFlags);
    if(result != S_OK)
    {
		fprintf(stderr, "Failed to enable video input. Is another application using the card?\n");
        goto bail;
    }

 
    result = deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, g_audioSampleDepth, g_audioChannels);
    if(result != S_OK)
    {
      fprintf(stderr, "Failed to enable audio input with 48kHz, depth %d, %d channels\n",
              g_audioSampleDepth, g_audioChannels);
        goto bail;
    }

	if (useSAGE) {
		// Get capture size
		image_width = displayMode->GetWidth();
		image_height = displayMode->GetHeight();
		if(useSBS)
		{
			sageW = image_width/2;
			sageH = image_height;

    		rgbImageData = (unsigned char*)malloc(image_width*image_height*3);
    		memset(rgbImageData, 0, image_width*image_height*3);
    		stride = sageW;	
		}
		else
		{
			sageW = image_width;
			sageH = image_height;
		}
		
		// Compute the actual framerate
		displayMode->GetFrameRate(&frameRateDuration, &frameRateScale);
		sageFPS = (double)frameRateScale / (double)frameRateDuration;
		fprintf(stderr, "GetFrameRate: %10ld %10ld --> fps %g\n", (long)frameRateScale, (long)frameRateDuration, sageFPS);

		// SAGE setup
		fprintf(stderr, "SAIL configuration was initialized by decklinkcapture.conf\n");
		fprintf(stderr, "\twidth %d height %d fps %g\n", sageW, sageH, sageFPS);

		if (pixelFormat != bmdFormat8BitYUV)
			fprintf(stderr, "SAGE works only with pixelFormat 1 (bmdFormat8BitYUV)\n");

		if(useSBS)
		{
			sageInf = createSAIL("decklinkcapture3d", sageW, sageH, PIXFMT_RGBS3D, NULL, TOP_TO_BOTTOM);
			sbsBuffer = nextBuffer(sageInf);
            sbsBuffer = (unsigned char*)malloc(sageW*sageH*6);
            memset(sbsBuffer, 0, sageW*sageH*6);
            sage::printLog("CAPTURE> SAGE: create buffer %d %d\n", image_width,image_height);
		}
		else
			sageInf = createSAIL("decklinkcapture3d", sageW, sageH, PIXFMT_YUV, NULL, TOP_TO_BOTTOM);
		sage::printLog("SAIL initialized");

		//////////////////////////////////
		#define BIG_AUDIO_BUFFER (10*1024*1024)
		char *audio_buffer;
		int buffer_length = BIG_AUDIO_BUFFER * g_audioChannels * (g_audioSampleDepth / 8);
		audio_buffer = (char*)malloc(buffer_length);
		memset(audio_buffer, 0, buffer_length);
		audio_ring = new Ring_Buffer(audio_buffer, buffer_length);
		fprintf(stderr, "Audio ring created\n");
		/////////////////////////////////

		//////////////////////////////////
		// SAM
		//////////////////////////////////
		if (useSAM) {
			char *displayModeString = NULL;
			char *inputString = NULL;
			const char *modelName = NULL;
			// Sadly, jack uses short names 
			char samName[32];
			memset(samName, 0, 32);
			displayMode->GetName((const char **) &displayModeString);
			deckLink->GetModelName(&modelName);
			inputString = connectionName(vport);
			snprintf(samName, 32, "[%s-%s] [%d-%d]", inputString, displayModeString, g_audioChannels, g_audioSampleDepth);
			free(displayModeString);

			sage::printLog("Using SAM IP: %s", samIP);

			m_sac = new sam::StreamingAudioClient(g_audioChannels, sam::TYPE_BASIC, samName, samIP, 7770);
			int returnCode = m_sac->start(0, 0, sageW, sageH, 0);

			if (physicalInputs)
			{
				unsigned int* inputs = new unsigned int[g_audioChannels];
				for (unsigned int n = 0; n < g_audioChannels; n++)
				{
					inputs[n] = n + 1;
				}
				sage::printLog("Calling SetPhysicalInputs");
				m_sac->setPhysicalInputs(inputs);
				delete[] inputs;
			}
			else {
				// register SAC callback
				m_sac->setAudioCallback(handle_sac_audio_callback, NULL);
			}

		}

		//////////////////////////////////
		uiLib.init((char*)"fsManager.conf");
		uiLib.connect(sageInf->Config().fsIP);
		uiLib.sendMessage(SAGE_UI_REG,(char*)" ");
		pthread_t thId;
		if(pthread_create(&thId, 0, readThread, (void*)&uiLib) != 0){
			sage::printLog("Cannot create a UI thread\n");
		}

	}

    result = deckLinkInput->StartStreams();
    if(result != S_OK)
    {
        goto bail;
    }

	// All Okay.
	exitStatus = 0;
	
	// Block main thread until signal occurs
	pthread_mutex_lock(&sleepMutex);
	pthread_cond_wait(&sleepCond, &sleepMutex);
	pthread_mutex_unlock(&sleepMutex);
	fprintf(stderr, "Stopping Capture\n");

bail:
   	
	if (videoOutputFile)
		close(videoOutputFile);
	if (audioOutputFile)
		close(audioOutputFile);
	
	if (displayModeIterator != NULL)
	{
		displayModeIterator->Release();
		displayModeIterator = NULL;
	}

    if (deckLinkInput != NULL)
    {
        deckLinkInput->Release();
        deckLinkInput = NULL;
    }

    if (deckLink != NULL)
    {
        deckLink->Release();
        deckLink = NULL;
    }

	if (deckLinkIterator != NULL)
		deckLinkIterator->Release();

    if (m_sac) delete m_sac;
    if (sageInf) deleteSAIL(sageInf);

    return exitStatus;
}


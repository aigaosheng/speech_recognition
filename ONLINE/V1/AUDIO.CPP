/****************************************************************
  FILE NAME: AUDIO.CPP

  DESCRIPTION: The file includes all functions related to operate
  the audio device, such as initializing the audio record device,
  allocating the record device, pausing the record device, resuming 
  the record device, and releasing the record device. And other 
  functions are about how to obtain Hamming window, extracting MEL
  cepstrum and calculating the differentiated feature.

  WRITTEN BY: XuBo
  MODIFIED BY: GaoSheng
  DATE: April 6,1999
****************************************************************/

#include "stdafx.h"
#include "const.h"
#include "global.h"
#include "audio.h"
#include "assert.h"
#include "waveproc.h"
#include "mmsystem.h"
#include "math.h"
//#include "dictation.h"
	
#define ENERGY_NORMALIZATION_IN_PROCESSING_BLOCK
#define	SHIFTBLOCK	7//9//1

//extern CDictation * pDictation;
//extern CModeManager  modeManager;
extern void	OnepassSearchEngine(float *pVector, short, short);
extern void	OnepassSearchEngineInit(void);


/////////////////////////////////////////////////////////////////////////////
// CAudioManage

CAudioManage::CAudioManage()
{
	int i;
	hWaveIn=NULL;					/* assume NULL */
	for(i=0;i < WAVEBUFNUM;i++)
	{
		hWaveBuf[i]=NULL;
		lpWaveBuf[i]=NULL;
		hWaveInHdr[i]=NULL;
		lpWaveInHdr[i]=NULL;		/* assume NULL */
		hWaveInInst[i]=NULL;
		lpWaveInInst[i]=NULL;		/* what does it use ? */
	}
	hPCMWaveFormat=NULL;
	lpPCMWaveFormat=NULL;
	hwndSampler=NULL;

	pVector = new float[NO_DIM * MAXPROCESSFRAMENUM];

	m_Buffer = new short[WAVEBUFSIZE + FRAMELEN];
	
	for(i=0;i<HFRAMELEN;i++)
		overlapSrcData[i]=0;
	
	featureStack = new S_FIFO[MAXFIFOITEMSIZE];
	for(i = 0; i < MAXFIFOITEMSIZE; i++)
	{
		featureStack[i].isFree = 1;
		featureStack[i].speechSegment = -1;
	}

	pHead = pTail = featureStack;
	localPos = 0;
	processingFramesNum = 0;
	processingWaveNum = 0;
	currentBufPos = 0;
	checkNoise = FALSE;
	noSpeech = 0;
	speechEnd = FALSE;
	speechBegin = FALSE;

	Eavg = 0;
	E_now = 0;
	audioState = 1; //the state of audio device is initialized zero.
//	firstSpeechBlock = 1;

	//高升编辑
	m_headLocalBuffer = m_blockCounter = m_curReadBlock = 0;
	m_startRecognizer = 0;
	memset(localEnMax, 0, sizeof(float)*MAX_BLOCK_START_RECOGNIZER);
	memset(localCepSum, 0, sizeof(float)*MAX_BLOCK_START_RECOGNIZER*FEATURE_DIM);
	processingData = new short[(MAX_BLOCK_START_RECOGNIZER*FRAMESPERBUF+1)*HFRAMELEN];
	m_silenceBlockCount = 0;
	m_preSpeechState = 0;
	m_longSilence = 0;
}

CAudioManage::~CAudioManage()
{
		//release the mem
	int i;
	delete []pVector;
	delete []m_Buffer;
	delete []processingData;
	delete []featureStack;
	CloseRecord();
}



void CAudioManage::ResumeRecord()

{
	
	char text[100];
	int bresult2;

	waveInReset(hWaveIn);

	// Only insert those marked with done
 	for(int Cycle = 0; Cycle < WAVEBUFNUM; Cycle++)
	{
	   	// Set up WAVEHDR structures and prepare them to get data from wave device.
   		lpWaveInHdr[Cycle]->lpData = (LPSTR)lpWaveBuf[Cycle];
   		lpWaveInHdr[Cycle]->dwBufferLength = WAVEBUFSIZE*sizeof(short);
   		lpWaveInHdr[Cycle]->dwFlags = 0L;
   		lpWaveInHdr[Cycle]->dwLoops = 0L;
   		lpWaveInHdr[Cycle]->dwUser = (DWORD) lpWaveInInst[Cycle];

   		if (bresult2=waveInPrepareHeader(hWaveIn, lpWaveInHdr[Cycle], sizeof(WAVEHDR)))
       	{
			//if(bresult2==MMSYSERR_INVALHANDLE)
			//	MessageBox("MMSYSERR_INVALHANDLE");
			//else if(bresult2==MMSYSERR_NOMEM)
			//	MessageBox("MMSYSERR_NOMEM ");
			//else
		     //   MessageBox("Unable To Prepare Wave Header!");
		
		    return;
	    }

	 	// send input buffer to waveform input device 
	 	if (waveInAddBuffer(hWaveIn, lpWaveInHdr[Cycle], sizeof(WAVEHDR)))
	  	{
		    waveInUnprepareHeader(hWaveIn, lpWaveInHdr[Cycle], sizeof(WAVEHDR));
//		    MessageBox("Failed to Send an Input buffer to device!");
			return ;
		}	
	} 

	bresult2 = waveInStart(hWaveIn);
	waveInGetErrorText(bresult2, text, 100);	
	//
	audioState = 2;
	//
}

BOOL CAudioManage::InitAudioDevice(void)
{
	if(InitDeviceCommon())
		if(InitDeviceIn())
			return TRUE;
	ClearDevice();	
	return FALSE;
}

void CAudioManage::GetFeature(int pointTemp)
{
	int frameTemp;

	frameTemp =  pointTemp / HFRAMELEN;
	CheckEndPoint(frameTemp);	
	Cepstrum(frameTemp - 1); //实际由原始数据仅能算出frameTmp-1帧
}
void CAudioManage::CheckEndPoint(int framesNum)
{
	int     i,j;
	long   	ptr;
	ptr=0L;

	for(i=0;i<framesNum;i++)
	{
		e[i]=0;
		for(j=0;j<HFRAMELEN;j++)
		{
			e[i]+=((double)m_Buffer[ptr]*(double)m_Buffer[ptr]);
			ptr++;
		}
	}
	return;
}	

void CAudioManage::Cepstrum(int td )

{   
	long ptr;
	int i,t;
	double fsmp[FRAMELEN],dat10,dat11;
	
	double e_max = 0;
	int tp = 0;

	//energy normalization when ReadFIFO.
#ifndef	ENERGY_NORMALIZATION_IN_PROCESSING_BLOCK
	for(t = tp; t < td; t++)
	{
		date = e[t] + e[t+1];
		if(date > e_max)
			e_max = date;
	}
#endif

	for(t = tp; t < td; t++)
	{
		for(i=0;i<PCEP;i++)
			fcep[t][i]=0.0;
			
#ifndef	ENERGY_NORMALIZATION_IN_PROCESSING_BLOCK
		fe[t][0]=(e[t]+e[t+1])/e_max;
#else
		fe[t][0]=(e[t]+e[t+1]);
#endif
		
		ptr=(long)t*HFRAMELEN;
		for (i=0;i<FRAMELEN;i++)     // hamming windowed  &  pre-emphasize
		{
		   dat10=m_Buffer[ptr]*hamming[i];
		   dat11=(double)m_Buffer[ptr+1]*hamming[i+1];
		   fsmp[i]=dat11-PRE*dat10;
		   ptr++;
		}

		int	iZero=CFilt(fsmp);
		if(iZero == -1)
		{
		 //  MessageBox("Energy is Zero in Processing");
		   return;
		}
		MFCC(t);
	}
	return;
}


void CAudioManage::CloseRecord()
{
	int i;
	waveInReset(hWaveIn);
	for(i=0;i<WAVEBUFNUM;i++)
		waveInUnprepareHeader(hWaveIn, (LPWAVEHDR)lpWaveInHdr[i], sizeof(WAVEHDR));
	waveInClose(hWaveIn);
	ClearDevice();
}


/**********************************
FUNCTION ClearDevice(HWND)
**********************************/
void CAudioManage::ClearDevice(void)
{
	int Cycle;
	// Clear Wave Format 
	GlobalUnlock(hPCMWaveFormat);
	GlobalFree(hPCMWaveFormat);
	// Clear Wave Buffer 
	for(Cycle=0;Cycle<WAVEBUFNUM;Cycle++)
	{
		GlobalUnlock(hWaveBuf[Cycle]);
		GlobalFree(hWaveBuf[Cycle]);
	}
	// Clear WaveIn header
	for(Cycle=0;Cycle<WAVEBUFNUM;Cycle++)
	{
		GlobalUnlock(hWaveInHdr[Cycle]);
		GlobalFree(hWaveInHdr[Cycle]);
	}
	return;
}

BOOL CAudioManage::LoadData()
{

	GenHamming();
	InitFilt(FiltCoe1,FiltCoe2,Num);
	CalDctCoef();
	CalFftCoef();

	return 0;
}


BOOL CAudioManage::InitDeviceCommon(void)
{
	int Cycle;
	
   	WORD             wDevNum;
	HANDLE           hWaveInCaps = NULL;
	LPWAVEINCAPS	 lpWaveInCaps;


	// Allocat and Lock memory for WAVEFORMAT structure.
	hPCMWaveFormat=GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
                              (DWORD)sizeof(PCMWAVEFORMAT));
   	if (!hPCMWaveFormat)
        {   
        	//MessageBox("Failed to Allocate Memory for WaveFormat Structure!");
            return FALSE ;
        }

   	lpPCMWaveFormat=(LPPCMWAVEFORMAT)GlobalLock(hPCMWaveFormat);
   	if (!lpPCMWaveFormat)
        {
            GlobalFree(hPCMWaveFormat);
            //MessageBox("Failed to Lock Memory for WAVEFORMAT Structure!");
            return FALSE ;
        }
                                                             
   // Setup proper format for recording & playing 
   	(lpPCMWaveFormat->wf).wFormatTag = WAVE_FORMAT_PCM;
   	(lpPCMWaveFormat->wf).nChannels  = 1;
   	(lpPCMWaveFormat->wf).nSamplesPerSec = FHZ;
   	(lpPCMWaveFormat->wf).nAvgBytesPerSec = BYTESIZE;
   	(lpPCMWaveFormat->wf).nBlockAlign = BITS/8;
   	lpPCMWaveFormat->wBitsPerSample=BITS;


   // Checking Existing of Sound Input device 
   wDevNum=waveInGetNumDevs();
   if (!wDevNum)
   		{	
   			//MessageBox("No Voice Input Device Existing!");
            return FALSE ;
        }
        
   // Allocat and lock memory for WAVEINCAPS structure.
   hWaveInCaps=GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (DWORD)sizeof(WAVEINCAPS));
   if (!hWaveInCaps)
   		{
            //MessageBox("Failed to Allocate Memory For WAVEINCAPS Structure!");
            return FALSE ;
       	}

   lpWaveInCaps=(LPWAVEINCAPS)GlobalLock(hWaveInCaps);
   if (!lpWaveInCaps)
   		{
            GlobalFree(hWaveInCaps);
            //MessageBox("Failed to Lock Memory for WAVEINCAPS Structure!");
            return FALSE ;
        }

   // Get capabilities of the device.
   if (waveInGetDevCaps((WORD)0, (LPWAVEINCAPS)lpWaveInCaps, (WORD)sizeof(WAVEINCAPS)))
   		{
            GlobalUnlock(hWaveInCaps);
            GlobalFree(hWaveInCaps);
            //MessageBox("Failed to Get Capabilities of the Device!");
            return FALSE;
        }

   // Make sure the device supports the specified foramt.
   if (!(lpWaveInCaps->dwFormats & WAVE_FORMAT_4M16))
        {
            GlobalUnlock(hWaveInCaps);
            GlobalFree(hWaveInCaps);
            //MessageBox("Device Does not Support the 4M16 Format");
            return FALSE ;
        }

   // We have done with WaveInCaps, free it.
   GlobalUnlock(hWaveInCaps);
   GlobalFree(hWaveInCaps);

   // make sure a wave input device supports this format.
   if (waveInOpen(NULL, (UINT)WAVE_MAPPER, (LPWAVEFORMATEX)lpPCMWaveFormat,
                          NULL, 0L, (DWORD)WAVE_FORMAT_QUERY))
   		{
            //MessageBox("录音初始化失败!");
            return FALSE;
        }
  // Initialize Record Buffer 
  for(Cycle=0;Cycle<WAVEBUFNUM;Cycle++)
		{
   		hWaveBuf[Cycle]=GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof(short)*WAVEBUFSIZE);
   		if (!hWaveBuf[Cycle])
			{
             //MessageBox("Failed to Allocate One of Data Buffer!");
		     return FALSE;
		    }

   		lpWaveBuf[Cycle]=(LPSTR)GlobalLock(hWaveBuf[Cycle]);
   		
   		if (!lpWaveBuf[Cycle])
	    
	       	{
		    //MessageBox("Failed to Lock One of Data Buffer!");
		    return FALSE;
	        }
		}
	return TRUE;
}

/****************************************************************************

FUNCTION: InitDeviceIn(HWND)

PURPOSE: Initialize sample device.

****************************************************************************/

BOOL CAudioManage::InitDeviceIn()
{
   int	Cycle;
   BOOL	bResult1;
   char text[100];

   int lklk=0;
   lklk = waveInGetNumDevs();

   bResult1 = FALSE;
   // open a waveform input device. 
   ::Sleep(100);
   bResult1=waveInOpen((LPHWAVEIN)&hWaveIn,(UINT)WAVE_MAPPER , (LPWAVEFORMATEX)lpPCMWaveFormat,
                         (DWORD)waveInProc,(DWORD)0L, (DWORD)CALLBACK_FUNCTION);
	if(bResult1)                         
        {  
			waveInGetErrorText(bResult1,text,100);	
			//MessageBox(text);
  			
        	//MessageBox("Can't Open the Waveform Device!");
//            exit(-5);
			return FALSE;
        }
   // Allocate and lock memory for the input waveform data.
 	for(Cycle=0;Cycle<WAVEBUFNUM;Cycle++)
	{
   			// Allocate and lock a waveform data headers.
   			hWaveInHdr[Cycle]=GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof(WAVEHDR));
	   		if (!hWaveInHdr[Cycle])
		   		{
				//MessageBox("Failed to Allocate Memory for Wave Data Header!");
	            return FALSE;
		        }

	   		lpWaveInHdr[Cycle] = (LPWAVEHDR) GlobalLock(hWaveInHdr[Cycle]);
	   		if (!lpWaveInHdr[Cycle])
	        	{
                //MessageBox("Failed to Lock Memory for Wave in Header!");
	           	return FALSE;
	        	}

	   	// Set up WAVEHDR structures and prepare them to get data from wave device.
	   		lpWaveInHdr[Cycle]->lpData = (LPSTR)lpWaveBuf[Cycle];
	   		lpWaveInHdr[Cycle]->dwBufferLength = WAVEBUFSIZE*sizeof(short);
	   		lpWaveInHdr[Cycle]->dwFlags = 0L;
	   		lpWaveInHdr[Cycle]->dwLoops = 0L;
	   		lpWaveInHdr[Cycle]->dwUser = (DWORD) lpWaveInInst[Cycle];

   		if (waveInPrepareHeader(hWaveIn, lpWaveInHdr[Cycle], sizeof(WAVEHDR)))
	      
       	{
		        //MessageBox("Unable To Prepare Wave Header!");
		        return FALSE;
	    }

	 	// send input buffer to waveform input device 
	 	if (waveInAddBuffer(hWaveIn, lpWaveInHdr[Cycle], sizeof(WAVEHDR)))
	  	{
		    waveInUnprepareHeader(hWaveIn, lpWaveInHdr[Cycle], sizeof(WAVEHDR));
		    //MessageBox("Failed to Send an Input buffer to device!");
			return FALSE;
		}	
	}
 	return TRUE;
}



void CAudioManage::InitFilt(double *FiltCoe1,double *FiltCoe2,int *Num)

{
  
	int		i,j;
	double	Freq;
	int		FiltFreq[FILTNUM+1]={0,100,200,300,400,500,600,700,800,900,1000,
			  1149,1320,1516,1741,2000,2297,2639,3031,3482,4000,
			  4595,5278,6063,6964,8001};
	int		BW[FILTNUM+1]={100,100,100,100,100,100,100,100,100,100,124,
				   160,184,211,242,278,320,367,422,484,556,
				   639,734,843,969,1112};


	for(i=0;i<=FFTLEN/2;i++)
	{
		Freq=(double)(FS*1000.0*(double)(i)/(double)(FFTLEN));
		for(j=0;j<FILTNUM;j++)
			{
			if(Freq>=(double)FiltFreq[j] && Freq<=(double)FiltFreq[j+1])
				{
				Num[i]=j;
				if(j == 0)
					FiltCoe1[i]=(double)0.0;
				else
					FiltCoe1[i]=((double)(FiltFreq[j]+BW[j])-Freq)/(double)(BW[j]);

				FiltCoe2[i]=(Freq-(double)(FiltFreq[j+1]-BW[j+1]))/(double)(BW[j+1]);
				FiltCoe1[i]=FiltCoe1[i]*FiltCoe1[i];
				FiltCoe2[i]=FiltCoe2[i]*FiltCoe2[i];
				break;
				}
			}
	}
}



void CAudioManage::GenHamming()

{

	int i;
	double twopi;
	twopi=8.*atan(1.);
	for (i=0;i<FRAMELEN+1;i++)
		hamming[i]=(double)(0.54-0.46*cos((double)i*twopi/(double)(FRAMELEN-1)));
	return;

}


void CAudioManage::SuspendRecord()

{
	waveInReset(hWaveIn);  // Changed from waveInStop
	//
	audioState = 0;
	//
	return;
}




void CAudioManage::MFCC(int & tidx)

{
	int idcep,iden;

	for(idcep=0;idcep<PCEP;idcep++)
	{
		fcep[tidx][idcep]=0.0;
		for(iden=1;iden<=FILTNUM;iden++)
		{
			fcep[tidx][idcep]=fcep[tidx][idcep]+En[iden]*DctCoef[idcep][iden];
		}
		fcep[tidx][idcep]=fcep[tidx][idcep]*0.1;
	}
}

/*
void CAudioManage::MFCCNew(int & tidx, double *en)

{
	int idcep,iden;

	for(idcep=0;idcep<PCEP;idcep++)
	{
		fcep[tidx][idcep]=0.0;
		for(iden=1;iden<=FiltNum;iden++)
		{
			fcep[tidx][idcep]=fcep[tidx][idcep]+en[iden]*DctCoef[idcep][iden];
		}
		fcep[tidx][idcep]=fcep[tidx][idcep]*0.1;
	}
}

*/

void CAudioManage::CalDctCoef()
{
	int idcep,iden;
	for(idcep=0;idcep<PCEP;idcep++)
	{
		for(iden=1;iden<=FILTNUM;iden++)
		{
			DctCoef[idcep][iden]=cos((idcep+1)*(iden-0.5)*PI/(FILTNUM));
		}
	}
}

int CAudioManage::CFilt(double *spdata)
{
	double real[FFTLEN+1],imgi[FFTLEN+1],temp;
	int id,id1,id2;

	for(id=0;id<FFTLEN;id++)
	{
		if(id<FRAMELEN)
			real[id+1]=spdata[id];
		else
			real[id+1]=(double)0.0;
		imgi[id+1]=(double)0.0;
	}
	FFT(real,imgi,0);

	for(id=0;id<=FILTNUM;id++)
		En[id]=(double)0.0;
	for(id=0;id<=FFTLEN/2;id++)
	{
		temp=real[id+1]*real[id+1]+imgi[id+1]*imgi[id+1];
		id1=Num[id];
		id2=id1+1;
		En[id1]=En[id1]+FiltCoe1[id]*temp;
		En[id2]=En[id2]+FiltCoe2[id]*temp;
	}

	for(id=1;id<=FILTNUM;id++)
	{
		if(En[id] == 0)
			return(-1);
		else
			En[id]=log(En[id]);
	}
	return(0);

}
 
int CAudioManage::CFiltDouble(double *spdata,double *spdatai)
{
/*	double temp;
	double TempReal,TempImage;
	int id,id1,id2;

	for(id=0;id<FFTLen;id++)
	{
		if(id<FrameLen)
		{
			real[id+1]=spdata[id];
			imgi[id+1]=spdatai[id];

		}
		else
		{
			real[id+1]=(double)0.0;
			imgi[id+1]=(double)0.0;
		}
	}
	FFT(real,imgi,0);

	for(id=0;id<=FiltNum;id++)
	{
		En[id]=(double)0.0;
		EnSecond[id]=(double)0.0;
	}

	for(id=0;id<=FFTLen/2;id++)
	{
		TempReal=0.5*(real[id+1]+real[FFTLen-id+1]);
		TempImage=0.5*(imgi[id+1]-imgi[FFTLen-id+1]);
		temp=TempReal*TempReal+TempImage*TempImage;
		id1=Num[id];
		id2=id1+1;
		En[id1]=En[id1]+FiltCoe1[id]*temp;
		En[id2]=En[id2]+FiltCoe2[id]*temp;
	}

	for(id=0;id<=FFTLen/2;id++)
	{
        TempReal=0.5*(imgi[id+1]+imgi[FFTLen-id+1]);
		TempImage=0.5*(real[FFTLen-id+1]-real[id+1]);
		temp=TempReal*TempReal+TempImage*TempImage;
		id1=Num[id];
		id2=id1+1;
		EnSecond[id1]=EnSecond[id1]+FiltCoe1[id]*temp;
		EnSecond[id2]=EnSecond[id2]+FiltCoe2[id]*temp;
	}

	for(id=1;id<=FiltNum;id++)
	{
		if(En[id] == 0 ||EnSecond[id] == 0 )
			return(-1);
		else
		{
			En[id]=log(En[id]);
			EnSecond[id]=log(EnSecond[id]);
		}
	}
*/	return(0);
}


/*
void CAudioManage::FFT(double *xr,double *xi,short inv)
{
	short m,nv2,i,j,nm1,k,l,le1,ip,n=FFTLen;
	double tr,ti,ur,ui,wr,wi,ur1,ui1;
	m=(short)(log((double)n)/log(2.)+.1);
	nv2=n/2;
	nm1=n-1;
	j=1;
	for(i=1;i<=nm1;++i)
	{
		if(i<j)
		{
			tr=xr[j];
			ti=xi[j];
			xr[j]=xr[i];
			xi[j]=xi[i];
			xr[i]=tr;
			xi[i]=ti;
		}
		k=nv2;
	R20:    if(k>=j) goto R30;
			j=j-k;
		k=(short)(k/2);
		goto R20;
	R30:    j=j+k;
	}
	for(l=1;l<=m;++l)
	{
		le1=(short)(pow(2,(l-1)));
		ur=1.0f;
		ui=0.f;
		wr=(float)cos(PI/(float)(le1));
		wi=-(float)sin(PI/(float)(le1));
		if(inv!=0) wi=-wi;
		for(j=1;j<=le1;++j)
		{
			for(i=j;i<=n;i=i+2*le1)
			{
				ip=i+le1;
				tr=xr[ip]*ur-xi[ip]*ui;
				ti=xr[ip]*ui+xi[ip]*ur;
				xr[ip]=xr[i]-tr;
				xi[ip]=xi[i]-ti;
				xr[i]=xr[i]+tr;
				xi[i]=xi[i]+ti;
			}
			ur1=ur*wr-ui*wi;
			ui1=ur*wi+ui*wr;
			ur=ur1;
			ui=ui1;
		}
	}
	if(inv == 0) return;
	for(i=1;i<=n;++i)
	{
		xr[i]=xr[i]/(float)(n);
		xi[i]=xi[i]/(float)(n);
	}
}
*/

void CAudioManage::FFT(double *xr,double *xi,short inv)
{
	short m,nv2,i,j,nm1,k,l,le1,ip,n=FFTLEN;
	double tr,ti,ur,ui,wr,wi,ur1,ui1;
	m=9;
	nv2=n/2;
	nm1=n-1;
	j=1;
	for(i=1;i<=nm1;++i)
	{
		if(i<j)
		{
			tr=xr[j];
			ti=xi[j];
			xr[j]=xr[i];
			xi[j]=xi[i];
			xr[i]=tr;
			xi[i]=ti;
		}
		k=nv2;
	R20:    if(k>=j) goto R30;
			j=j-k;
		k=(short)(k/2);
		goto R20;
	R30:    j=j+k;
	}
	for(l=1;l<=m;++l)
	{
		le1=(short)(pow(2,(l-1)));
		ur=1.0f;
		ui=0.f;
		wr=FftCoefr[l];//(float)cos(PI/(float)(le1));
		wi=FftCoefi[l];//-(float)sin(PI/(float)(le1));
		if(inv!=0) wi=-wi;
		for(j=1;j<=le1;++j)
		{
			for(i=j;i<=n;i=i+2*le1)
			{
				ip=i+le1;
				tr=xr[ip]*ur-xi[ip]*ui;
				ti=xr[ip]*ui+xi[ip]*ur;
				xr[ip]=xr[i]-tr;
				xi[ip]=xi[i]-ti;
				xr[i]=xr[i]+tr;
				xi[i]=xi[i]+ti;
			}
			ur1=ur*wr-ui*wi;
			ui1=ur*wi+ui*wr;
			ur=ur1;
			ui=ui1;
		}
	}
	if(inv == 0) return;
	for(i=1;i<=n;++i)
	{
		xr[i]=xr[i]/(float)(n);
		xi[i]=xi[i]/(float)(n);
	}
}

void CAudioManage::CalFftCoef()
{
	short l,le1;
	for(l=1;l<=9;++l)
	{
		le1=(short)(pow(2,(l-1)));
		FftCoefr[l]=(float)cos(PI/(float)(le1));
		FftCoefi[l]=-(float)sin(PI/(float)(le1));
	}
}


void CAudioManage::GetDifFeature(int startFrame, int Length)
{
	int j,ii;
	float *pVectorTmp = pVector + startFrame * NO_DIM;

	// Calculate Differential Feature
	for(j = 2; j < (Length - 2); j++)
		for(ii = 0; ii < FEATURE_DIM; ii++)
			pVectorTmp[(j * NO_DIM) + (ii + FEATURE_DIM)] = pVectorTmp[(j + 2) * NO_DIM + ii] - pVectorTmp[(j - 2) * NO_DIM + ii];
			
	for(ii = 0; ii < FEATURE_DIM; ii++)    {
		pVectorTmp[(Length - 2) * NO_DIM + (ii + FEATURE_DIM)] = pVectorTmp[(Length - 3) * NO_DIM + (ii + FEATURE_DIM)];
		pVectorTmp[(Length - 1) * NO_DIM + (ii + FEATURE_DIM)] = pVectorTmp[(Length - 3) * NO_DIM + (ii + FEATURE_DIM)];
    }
			
	for(ii = 0; ii < FEATURE_DIM; ii++)	{
		pVectorTmp[(0 * NO_DIM) + (ii + FEATURE_DIM)] = pVectorTmp[(2 * NO_DIM) + (ii + FEATURE_DIM)];				
		pVectorTmp[(1 * NO_DIM) + (ii + FEATURE_DIM)] = pVectorTmp[(2 * NO_DIM) + (ii + FEATURE_DIM)];				
	}

	// calculate 2 differential energy
	for(j = 2; j < (Length - 2); j++)
		for(ii = 0; ii < FEATURE_DIM; ii++)
			pVectorTmp[(j * NO_DIM) + (ii + 2 * FEATURE_DIM)] = pVectorTmp[(j + 2) * NO_DIM + (ii + FEATURE_DIM)] - pVectorTmp[(j - 2) * NO_DIM + (ii + FEATURE_DIM)];
			
	for(ii = 0; ii < FEATURE_DIM; ii++)    {
		pVectorTmp[(Length - 2) * NO_DIM + (ii + 2 * FEATURE_DIM)] = pVectorTmp[(Length - 3) * NO_DIM + (ii + 2 * FEATURE_DIM)];
		pVectorTmp[(Length - 1) * NO_DIM + (ii + 2 * FEATURE_DIM)] = pVectorTmp[(Length - 3) * NO_DIM + (ii + 2 * FEATURE_DIM)];
    }
			
	for(ii = 0; ii < FEATURE_DIM; ii++)	{
		pVectorTmp[(0 * NO_DIM) + (ii + 2 * FEATURE_DIM)] = pVectorTmp[(2 * NO_DIM) + (ii + 2 * FEATURE_DIM)];				
		pVectorTmp[(1 * NO_DIM) + (ii + 2 * FEATURE_DIM)] = pVectorTmp[(2 * NO_DIM) + (ii + 2 * FEATURE_DIM)];				
	}


	return;

}

BOOL	CAudioManage::WriteFIFO(short *data)
{
	int	i, j, frameCt, frameNum, firstFrame, lastFrame, k;
	int curHead, curTail;
	float pitchBuf[MAXPROCESSFRAMENUM];
	float emax;
	float averageCepstral[FEATURE_DIM];

//	FILE *fp=fopen("debug.txt", "a+t");
	
	if(pHead == pTail && !pHead->isFree) 
	{
		if(speechEnd)
		{
			speechBegin = 0;
			speechEnd = 0;
			noSpeech = 0;
			currentBufPos = 0;
			Eavg = 0;
//			firstSpeechBlock = 1;
		}
		//fprintf(fp,"flown\n");
			//fclose(fp);
		return FIFO_FLOW; //FIFO is full
	}
	if(!pHead->isFree)
	{
		if(speechEnd)
		{
			speechBegin = 0;
			speechEnd = 0;
			noSpeech = 0;
			currentBufPos = 0;
			Eavg = 0;
//			firstSpeechBlock = 1;
		}
		//fprintf(fp,"flown\n");
		//	fclose(fp);
		return FIFO_FLOW;
	}

//FILE *fp;
//fp=fopen("cep.dat", "a+b");
//fp=fopen("cep.txt", "a+t");
	//第一帧的开始半帧数据补零，以满足每次提取的特帧数据数目相同
//	if(firstSpeechBlock) //initialize to 1
//	{
//		memset(overlapSrcData, 0, sizeof(short) * HFRAMELEN);
//		firstSpeechBlock = 0;
//	}

	if(data == NULL)
		m_longSilence = 1;

	if(speechEnd)
	{
		speechBegin = 0;
		speechEnd = 0;
		noSpeech = 0;
		currentBufPos = 0;
		Eavg = 0;
	}

	/*
FILE *fsrc=fopen("src.dat","a+b");
	fwrite(data, sizeof(short), WAVEBUFSIZE, fsrc);
	fclose(fsrc);

	fsrc=fopen("src.idx","a+b");
	fwrite(&m_longSilence,sizeof(char),1,fsrc);
	fclose(fsrc);
*/
	if(data != NULL)
	{	
/*FILE *fsrc=fopen("srcgsh.dat","a+b");
	fwrite(data, sizeof(short), WAVEBUFSIZE, fsrc);
	fclose(fsrc);

	fsrc=fopen("srcgsh.idx","a+b");
	fwrite(&m_longSilence,sizeof(char),1,fsrc);
	fclose(fsrc);
*/
		//以下两行代码仅用于保留原始语音数据
//		memcpy(waveData+processingWaveNum*HFRAMELEN, data, sizeof(short)*WAVEBUFSIZE);
//		processingWaveNum += FRAMESPERBUF;
		//


		//add overlap frame data between two ajacent blocks
		memcpy(m_Buffer, overlapSrcData, sizeof(short)*HFRAMELEN);
		memcpy(m_Buffer+HFRAMELEN, data, sizeof(short)*WAVEBUFSIZE);
		memset(m_Buffer+WAVEBUFSIZE+HFRAMELEN, 0, sizeof(short)*HFRAMELEN);
		GetFeature(WAVEBUFSIZE + HFRAMELEN);
		//store the overlap source data
		memcpy(overlapSrcData, m_Buffer+WAVEBUFSIZE, sizeof(short)*HFRAMELEN);
		frameCt = FRAMESPERBUF;

		//识别引擎等待语音数据累积到N块被启动
		curTail = (m_headLocalBuffer + m_blockCounter) % MAX_BLOCK_START_RECOGNIZER; //计算尾指针
		memcpy(m_localDataBuffer + curTail * WAVEBUFSIZE, data, sizeof(short) * WAVEBUFSIZE); //把原始数据写入本地buffer
		//
		localEnMax[curTail] = 0.0;
		for(j = 0; j < FEATURE_DIM; j++)
			localCepSum[curTail][j] = 0.0;
		//
		for(i = 0; i < frameCt; i++)
		{
			m_localFeatureBuffer[curTail*FRAMESPERBUF+i][0] = fe[i][0]; //把绝对能量写入本地buffer
			if(fe[i][0] > localEnMax[curTail]) //计算本块的能量最大值
				localEnMax[curTail] = fe[i][0];
			//把倒谱写入本地buffer
			for(j = 0; j < PCEP; j++)
			{
				m_localFeatureBuffer[curTail*FRAMESPERBUF+i][j+1] = (float)fcep[i][j];
				//计算倒谱和
				localCepSum[curTail][j+1] += (float)fcep[i][j];
			}
		}
		m_blockCounter++;
		m_curReadBlock++;	
	}

	if(!m_longSilence && m_blockCounter < MAX_BLOCK_START_RECOGNIZER)
		return FIFO_OPERATION_NORMAL;

	if(m_curReadBlock <= 0)
	{
		if(pHead != pTail)
		{
			if(pHead == featureStack)
				(featureStack+ MAXFIFOITEMSIZE - 1)->speechSegment = 2;
			else
				(pHead-1)->speechSegment = 2;
		}
		Reset();
		return FIFO_OPERATION_NORMAL;
	}

	frameNum = m_blockCounter * FRAMESPERBUF;
	//计算最大能量和倒谱和
	emax = 0.0;
	for(i = 0; i < FEATURE_DIM; i++)
		averageCepstral[i] = 0.0;

	memset(processingData, 0, sizeof(short)*HFRAMELEN);
	for(i = 0; i < m_blockCounter; i++)
	{
		curHead = (m_headLocalBuffer + i) % MAX_BLOCK_START_RECOGNIZER;
		if(localEnMax[curHead] > emax)
			emax = localEnMax[curHead];
		//计算倒谱和
		for(j = 1; j < FEATURE_DIM - 1; j++)
			averageCepstral[j] += localCepSum[curHead][j];
		//拷贝数据
		memcpy(processingData+HFRAMELEN+i*WAVEBUFSIZE, m_localDataBuffer+curHead*WAVEBUFSIZE, sizeof(short)*WAVEBUFSIZE);
	}
	//计算平均倒谱
	for(i = 1; i < FEATURE_DIM - 1; i++)
		averageCepstral[i] /= frameNum;
	//提取基频
	for(i = 0; i < frameNum; i++)
		pitchBuf[i] = 1.0;
	//SetPitchExchangeBuf(processingData, frameNum * HFRAMELEN, pitchBuf);
	SetPitchExchangeBuf(processingData, (frameNum+1)* HFRAMELEN, pitchBuf);
	ExtractPitch();
	//由当前的数据的数据计算出的基频比倒谱少1帧
//	pitchBuf[frameNum-1] = pitchBuf[frameNum-2];

	//
	int count = 0;
	for(i = 0; i < m_blockCounter; i++)
	{
		curHead = (m_headLocalBuffer + i) % MAX_BLOCK_START_RECOGNIZER;
		for(k = 0; k < FRAMESPERBUF; k++)
		{	
			//能量归一化和取对数
			pVector[count*NO_DIM] = m_localFeatureBuffer[curHead*FRAMESPERBUF+k][0];
			pVector[count*NO_DIM] = pVector[count*NO_DIM] / emax;
			if(pVector[count*NO_DIM] < 1.0e-5)
				pVector[count*NO_DIM] = (float)1.0e-5;
			pVector[count*NO_DIM] = log10(pVector[count*NO_DIM]) / 5.0;
			//倒谱归一化
			for(j = 1; j < FEATURE_DIM-1; j++)
				pVector[count*NO_DIM+j] = m_localFeatureBuffer[curHead*FRAMESPERBUF+k][j] - averageCepstral[j];
			//基频
			pVector[count*NO_DIM+FEATURE_DIM-1] = pitchBuf[count];
			count++;
		}
	}
	/*
	for(i = 0; i < frameNum; i++)
	{
		pVector[i*NO_DIM] = pVector[i*NO_DIM] / emax;
		if(pVector[i*NO_DIM] < 1.0e-5)
			pVector[i*NO_DIM] = (float)1.0e-5;
		pVector[i*NO_DIM] = log10(pVector[i*NO_DIM]) / 5.0;
		//倒谱归一化
		for(j = 1; j < FEATURE_DIM - 1; j++)
			pVector[i*NO_DIM+j] -= averageCepstral[j];
		pVector[i*NO_DIM+FEATURE_DIM-1] = pitchBuf[i];
	}
	*/
	ASSERT(count == frameNum);

	if(m_startRecognizer)
	{
		GetDifFeature(frameNum-FRAMESPERBUF*m_curReadBlock-4, FRAMESPERBUF*m_curReadBlock+4); // cause the feature changing of the neighbouring frames
		//if(!speechEnd)
		if(!m_longSilence)
		{
			ASSERT(m_blockCounter == MAX_BLOCK_START_RECOGNIZER);
			m_headLocalBuffer = (m_headLocalBuffer + m_curReadBlock) % MAX_BLOCK_START_RECOGNIZER; //本地数据块下移一块
			m_blockCounter = m_blockCounter - m_curReadBlock; //SHIFTBLOCK; //数据块计数器减一
			//
			firstFrame = frameNum - FRAMESPERBUF * m_curReadBlock - 2;
			lastFrame = frameNum - 2;
		}
		else
		{
			firstFrame = frameNum - FRAMESPERBUF * m_curReadBlock - 2;
			lastFrame = frameNum;
		}
	}
	else
	{
		GetDifFeature(0, frameNum); // cause the feature changing of the neighbouring frames

//		if(!speechEnd)
		if(!m_longSilence)
		{
			ASSERT(m_blockCounter == MAX_BLOCK_START_RECOGNIZER);
			m_headLocalBuffer = (m_headLocalBuffer + SHIFTBLOCK) % MAX_BLOCK_START_RECOGNIZER; //本地数据块下移一块
			m_blockCounter = m_blockCounter - SHIFTBLOCK; //数据块计数器减一
			//
			firstFrame = 0;
			lastFrame = frameNum - 2;
		}
		else
		{
			firstFrame = 0;
			lastFrame = frameNum;
		}
	}
	m_curReadBlock = 0;

	//写入FIFO
//	FILE *fp=fopen("cep.dat","a+b");

	for(i = firstFrame; i < lastFrame; i++) 
	{   
		pHead->isFree = 0; //set it bussy
		pHead->speechSegment = 1;
//		if(i==0 && (frameCt == FRAMESPERBUF-1))
//			pHead->speechSegment=0;		// The first frame of the speech

		if(i == lastFrame - 1 && m_longSilence)//speechEnd)//
			pHead->speechSegment = 2;

		for(j = 0; j < NO_DIM; j++)
			pHead->vector[j] = pVector[i*NO_DIM+j];


//fwrite(pHead->vector, sizeof(float), NO_DIM, fp);

		//move head pinter to next item
		if(pHead == featureStack + MAXFIFOITEMSIZE - 1)
			pHead = featureStack; //if current item is the bottom of FIFO, next item which pHead points to is the top of FIFO
		else
			pHead++;

		if(pHead == pTail && !pHead->isFree)
		{
			//fprintf(fp,"flown\n");
			//fclose(fp);
			return FIFO_FLOW;
		}
	}

//fclose(fp);

	if(!m_startRecognizer)
	{
		SetEvent(g_hEvent);
		eventCounter = InterlockedIncrement(&eventCounter); // 高生编辑
		m_startRecognizer = 1;
	}


	if(m_longSilence)
	{
		//高升编辑
		SetEvent(g_hEvent);
		eventCounter = InterlockedIncrement(&eventCounter); // 高生编辑
		Reset();
	}
	return FIFO_OPERATION_NORMAL;
}

BOOL	CAudioManage::ReadFIFO(void)
{
	char exitLoopSpeechEnd, exitLoopFifoNull;
	float frameVector[NO_DIM];

	if(pHead == pTail && pHead->isFree) 
		return FIFO_NULL; //FIFO is null

	if(pTail->isFree)
		return FIFO_NULL;

	exitLoopSpeechEnd = 0;
	exitLoopFifoNull = 0;

	memcpy(frameVector, pTail->vector, sizeof(float) * NO_DIM);
	pTail->isFree = 1;
/*/
FILE*fp=fopen("cepp.dat","a+b");
fwrite(frameVector,sizeof(float),NO_DIM,fp);
fclose(fp);
/*/
	if(pTail->speechSegment == 2)
		exitLoopSpeechEnd = 1;	
	
	if(pTail == featureStack + MAXFIFOITEMSIZE - 1)
		pTail = featureStack;
	else
		pTail++;

	if(pTail == pHead)
		exitLoopFifoNull = 1; //all features are read or speech segment occurs.
//	OnepassSearchEngine(frameVector, exitLoopFifoNull | exitLoopSpeechEnd);// | exitLoopFifoNull);
	OnepassSearchEngine(frameVector, exitLoopFifoNull, exitLoopSpeechEnd);// | exitLoopFifoNull);

	return FIFO_OPERATION_NORMAL;
}

int	CAudioManage::ReadFIFONoshift(float *frameVector)
{
	char exitLoopSpeechEnd, exitLoopFifoNull;
//	float frameVector[NO_DIM];

	if(pHead == pTail && pHead->isFree) 
		return FIFO_NULL; //FIFO is null

	if(pTail->isFree)
		return FIFO_NULL;

	memcpy(frameVector, pTail->vector, sizeof(float) * NO_DIM);

	return FIFO_OPERATION_NORMAL;
}

// Differences with ReadFIFO
// 1.This function reads data from the top to the bottom. So it demands 
//   that writing data always from the top and before the next sentence
//   begins, reset the two pointers of FIFO
//
BOOL	CAudioManage::ReadFifoAdapt(void)
{
	char exitLoopSpeechEnd, exitLoopFifoNull;
	float frameVector[NO_DIM];

	if(pHead == pTail && pHead->isFree) 
		return FIFO_NULL; //FIFO is null

	if(pTail->isFree)
		return FIFO_NULL;

	exitLoopSpeechEnd = 0;
	exitLoopFifoNull = 0;

	memcpy(frameVector, pTail->vector, sizeof(float) * NO_DIM);
	pTail->isFree = 1;
	//保存特征
	memcpy(featureVector+processingFramesNum, frameVector, sizeof(float) * NO_DIM);
	processingFramesNum++;
/*/
FILE*fp=fopen("cepp.dat","a+b");
fwrite(frameVector,sizeof(float),NO_DIM,fp);
fclose(fp);
/*/
	if(pTail->speechSegment == 2)
		exitLoopSpeechEnd = 1;	
	
	if(pTail == featureStack + MAXFIFOITEMSIZE - 1)
		pTail = featureStack;
	else
		pTail++;

	if(pTail == pHead)
		exitLoopFifoNull = 1; //all features are read or speech segment occurs.
//	OnepassSearchEngine(frameVector, exitLoopFifoNull | exitLoopSpeechEnd);// | exitLoopFifoNull);
	OnepassSearchEngine(frameVector, exitLoopFifoNull, exitLoopSpeechEnd);// | exitLoopFifoNull);

	return FIFO_OPERATION_NORMAL;
}


/*
BOOL CAudioManage::FeatureExtract(short *lpDataS,int sLen,float *lpDataF, int *segFlag)
{

	int	i, j, E[1500], dat0 , nFrames;
	//CAudioManage	*audioManager;    //zj delete useless		5.12

	nFrames=sLen/HFRAMELEN;

	if(!checkNoise) 
	{
		bias = 0;
		for(i = 0; i < sLen; i++)
			bias += lpDataS[i];
		bias /= sLen;
	}
	else
	{
		for(i = 0; i < sLen; i++)
			lpDataS[i] -= bias;
	}

	//calculate energy of each frame and total energy of all frames in the recording buffer
	E_now = 0;
	for(i = 0; i < nFrames; i++)
	{
		E[i] = 0L;
		for(j = 0; j < HFRAMELEN; j++)
		{
			dat0 = lpDataS[i*HFRAMELEN+j];
			E[i] += abs(dat0);
		}
		E_now += E[i];
	}
	//
	if(!checkNoise)
	{
		E_start = MAX_BEGINE_AMPLITUDE * E_now;
		E_end = MAX_END_AMPLITUDE * E_now;
		checkNoise = TRUE;
	}
	else
    {
		if(!speechBegin) //initialized value is 0
		{
			if(E_now > E_start) 
			{
				speechBegin = TRUE; //speech data begins
				Eavg = E_now;
			}
			else				// updates the threshold 
			{
				E_start = MAX_BEGINE_AMPLITUDE * E_now; //dramatically modify the threshold of speech start
				E_end = MAX_END_AMPLITUDE * E_now;//dramatically modify the threshold of speech end
			}
		}
		else
		{
			if(E_now > E_end)
			    noSpeech = 0;
			else
				noSpeech++;
		
			if(noSpeech >= 1)
			{
				speechEnd = TRUE;	
			}
			Eavg += E_now;
		}
	}
	
	currentBufPos += sLen;

	//add overlap frame data between two ajacent blocks
	// Process the feature frame by frame
	for(i=0;i<nFrames;i++)
	{

	memcpy(m_Buffer, overlapSrcData, sizeof(short)*HFRAMELEN);
	memcpy(m_Buffer+HFRAMELEN, lpDataS+i*HFRAMELEN, sizeof(short)*HFRAMELEN);
	GetFeature(2*HFRAMELEN);
	memcpy(overlapSrcData, lpDataS+i*HFRAMELEN, sizeof(short)*HFRAMELEN);

	*lpDataF ++  = (float)fe[0][0];

	for(j = 0; j < PCEP; j++)
		*lpDataF ++ = (float)fcep[0][j];
	}

	if(!speechBegin)

	{
		for(i=0;i<nFrames;i++)
			*segFlag++=-1;
	}

	else
	{
		for(i=0;i<nFrames;i++)
		{
			*segFlag=1;

			if(i==0 && firstSpeechBlock)
			{
				*segFlag=0;					
				firstSpeechBlock=0;			
			}
			if(i==nFrames-1 && speechEnd)
				*segFlag=2;
			segFlag++;
		}
	}


	if(speechEnd)
	{
		speechBegin = 0;
		speechEnd = 0;
		noSpeech = 0;
		currentBufPos = 0;
		Eavg = 0;
		firstSpeechBlock = 1;
	}

	return 0;
}
*/

void CAudioManage::Reset()
{
	//高升编辑
	memset(overlapSrcData, 0, sizeof(short)*HFRAMELEN);
	m_headLocalBuffer = m_blockCounter = m_longSilence = 0;
	m_silenceBlockCount = 0;
	m_startRecognizer = 0;
	m_curReadBlock = 0;
	memset(localEnMax, 0, sizeof(float)*MAX_BLOCK_START_RECOGNIZER);
	memset(localCepSum, 0, sizeof(float)*MAX_BLOCK_START_RECOGNIZER*FEATURE_DIM);
}
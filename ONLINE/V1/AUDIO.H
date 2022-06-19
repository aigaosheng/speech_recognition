// audio.h : header file
//
#ifndef  _AUDIO_H_
#define  _AUDIO_H_

#include "mmsystem.h"
//#include "typedef.h"

typedef struct waveInst {
    HANDLE hWaveInst;
    HANDLE hWaveHdr;
    HANDLE hWaveData;
} WAVEINST;
typedef WAVEINST FAR *LPWAVEINST;

/*typedef struct tagFifoItem
{
char	isFree; //0: busy; 1:free
char	speechSegment;
float	cep[FramesPerBuf][PCEP+1];
short  srcData[dwWaveBufSize];
}S_FIFO;
*/
typedef struct tagFifoItem
{
	char	isFree; //0: busy; 1:free
	char	speechSegment;
	//float	cep[PCEP+2];
	float	vector[NO_DIM];
	//short  srcData[HFRAMELEN];//[dwWaveBufSize];
}S_FIFO; //高声编辑

class CAudioManage
{
	// Construction
public:
	CAudioManage();
	
	// Attributes
public:
	
//	int		m_nCount;
//	float	pitchBuf[FRAMESPERBUF];
	
	double	hamming[FRAMELEN+1];
	double	FiltCoe1[FFTLEN/2+1], FiltCoe2[FFTLEN/2+1], En[FILTNUM+1], EnSecond[FILTNUM+1];
	double	DctCoef[PCEP][FILTNUM+1];
	double	FftCoefr[10], FftCoefi[10];
	int		Num[FFTLEN/2+1];
	double 	fcep[FRAMESPERBUF][PCEP];//MaxRegBlockFrameNum
	//	double	cep[PCEP];
	double 	fe[FRAMESPERBUF][PEDE], Eprev;
	double	Ew, Es;
	//double *	pMainCep;
	//	int		Bias;
	//	long	BiasEstimation;
	double fsmp[FRAMELEN], fsmpsecond[FRAMELEN];
	double real[FFTLEN+1], imgi[FFTLEN+1];
	
	double e[FRAMESPERBUF+2];//MaxRegBlockFrameNum];
	
	int		checkNoise, noSpeech, speechEnd, speechBegin, bias, 
			m_silenceBlockCount,
			m_preSpeechState; //保存speech end之后的上一次speech begin的标志
	long	Eavg, E_start, E_end, E_now;
	S_FIFO  *featureStack;
	
	float	*pVector;//[No_Dim];
	S_FIFO  *pHead, *pTail, *pStartTail,*pTemp, *pTimePos;
	int		localPos, processingFramesNum, currentBufPos, processingWaveNum;
	//	short	processData[(MaxRegBlockFrameNum+1)*hFrameLen];
	int		audioState;//store the state of the audio device
	short	overlapSrcData[HFRAMELEN];
//	char	firstSpeechBlock;
	//
//	short	waveData[MAXREGBLOCKFRAMENUM*HFRAMELEN];//[No_Dim];
	
//	short	timeMark1,timeMark2;
	//高升编辑
	short	m_headLocalBuffer, m_blockCounter, m_curReadBlock; //管理本地buffer的指针
	char	m_startRecognizer; //识别器引擎启动的标志。0：未启动； 1：启动
	short	m_localDataBuffer[MAX_BLOCK_START_RECOGNIZER*WAVEBUFSIZE]; //本地的原始语音数据buffer
	float	m_localFeatureBuffer[MAX_BLOCK_START_RECOGNIZER*FRAMESPERBUF][FEATURE_DIM];
	float	localEnMax[MAX_BLOCK_START_RECOGNIZER], localCepSum[MAX_BLOCK_START_RECOGNIZER][FEATURE_DIM];
	short	*processingData;
	char	m_longSilence;
	float	featureVector[NO_DIM*MAX_SPEECH_LENGTH];
	short	waveData[MAX_SPEECH_LENGTH*HFRAMELEN];
	//private:
public:
	HWAVEIN     	hWaveIn;					/* assume NULL */
	long			Count,CountLimit;
	long			MaxSize,PlaySize;
	double			Emax;
	/* for common input & output device use */
	HANDLE         	hWaveBuf[WAVEBUFNUM];
	LPSTR          	lpWaveBuf[WAVEBUFNUM];
	HANDLE         	hWaveInHdr[WAVEBUFNUM];
	LPWAVEHDR   	lpWaveInHdr[WAVEBUFNUM];		/* assume NULL */
	HANDLE			hWaveInInst[WAVEBUFNUM];
	LPWAVEINST     	lpWaveInInst[WAVEBUFNUM];		/* what does it use ? */
	HANDLE      	hPCMWaveFormat;
	LPPCMWAVEFORMAT 	lpPCMWaveFormat;
	HWND			hwndSampler;
	
private:
	
	short * m_Buffer;
	// Operations
public:
	BOOL LoadData();
	BOOL InitDeviceIn();
	BOOL InitDeviceCommon(void);
	void ClearDevice(void);
	void ResumeRecord();		// Start for Speech Recognition
	void CloseRecord();		// End for Speech Recognition
	void SuspendRecord();	
	void GenHamming(void);
	void InitFilt(double *FiltCoe1,double *FiltCoe2,int *Num);
	void GetFeature(int);
	void FFT(double *xr,double *xi,short inv);
	void Cepstrum(int BufferLength);
	
	void MFCC(int & tidx);
	//void MFCCNew(int & tidx, double *en);
	void CalDctCoef();
	void CalFftCoef();
	int CFilt(double *spdata);
	int CFiltDouble(double *spdata,double *spdatai);
	void CheckEndPoint(int);
	void GetDifFeature(int, int Length);
	//BOOL FeatureExtract(short *lpDataS,int sLen,float *lpDataF, int *segFlag);
	//Appended by GS. Sept 25, 1998
	BOOL InitAudioDevice(void);
	//Appended by GS on March 25,1999
	BOOL	WriteFIFO(short*);
	BOOL	ReadFIFO(void);
	BOOL	ReadFIFONoshift(float*);
	//Appended by GS on July 1, 1999
	BOOL	ReadFifoAdapt(void);
	void	Reset(void);
	// Implementation
public:
	virtual ~CAudioManage();
};

/////////////////////////////////////////////////////////////////////////////
#endif


// Recognize.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#define RECOGNIZE_EXPORTS
#include "Recognize.h"

#include "const.h"
#include "global.h"
#include	"modeManager.h"


extern void	OnepassSearchEngineInit(void); //高升编辑
//
DWORD	m_nHisWord1 = HEAD_WORD, m_nHisWord2 = HEAD_WORD; //高升添加
long	eventCounter = 0;
char	amTopic[50];
int totalFrame=0;

//
CAM				*g_pAM = NULL;
CLM				*g_pLM = NULL;
//上面的两个类由柯文编辑

CLextree		*g_pLextree = NULL;
CWordLattice	*g_pWordLattice = NULL;
//上面的两个类是由孟磊编辑

CPath			*g_pPath = NULL;
//由王胜君编辑

CAudioManage *g_pAudio = NULL;

HANDLE g_hEvent = NULL;
BOOL g_fTerminate = FALSE;
HANDLE g_RecogThread = NULL, g_stimulateThread=NULL;

HWND g_hMsgTarget = NULL;

char *g_stHanziString[MAX_MODE_NUM];

DWORD WINAPI RecogThreadProc(LPVOID lpParam);
int RecogThreadProc(void);
DWORD WINAPI RecogThreadProcAdapt(LPVOID lpParam);
DWORD WINAPI StimulateThreadProc(LPVOID lpParam);
extern int timeAxis;

CModeManager *g_pModeManager = NULL;

char  m_firmWords[MAX_SENTENCE_CHARACTERS];


//下面的函数是用来输出句子的，通过  WOrdLattice传过来的词序列数组进行句子的输出
//在这里词序列中装载的是词典中词的序号，所以需要给定此点数组
//wsj 2000.7.18
void GetLastFirmwords()
{

	int i, j;
	char chtmp[30]="";
	int senLen, pWordSeq[MAX_SENTENCE_LENGTH];
	char *stHanziString = g_stHanziString[g_pModeManager->GetActiveMode()];

	g_pWordLattice->GetWordSequence(senLen, pWordSeq);
	
	strcpy(m_firmWords, "");
	//在这里句子序列与实际的输出是相反的，所以应该倒序输出
	if(senLen > 0){
		for(i = senLen - 1; i >= 0 ; i--)	{
			for(j = 0; j < MAX_WORD_LENGTH * 2; j++)
				chtmp[j] = stHanziString[pWordSeq[i] * MAX_WORD_LENGTH * 2 + j];
			strcat(m_firmWords, chtmp);
		//	strcat(m_firmWords, "/");			
			//fprintf(OutFile, "%d %d %d,", wordSeq[i], timeSeq[i], posSeq[i]);
		}
	//	printf("%s\n", m_firmWords);
		//fprintf(OutFile, "%s\n", sentence);
		//下面代码由高生编辑
		if(g_pModeManager->GetActiveMode() == COMMAND_MODE)
		{
			m_nHisWord1 = HEAD_WORD;
			m_nHisWord2 = HEAD_WORD;
		}
		else
		{
			if(senLen >= 2)
			{
				m_nHisWord1 = pWordSeq[1];//HEAD_WORD;//
				m_nHisWord2 = pWordSeq[0];//HEAD_WORD;//
			}
			else
			{
				m_nHisWord1 = HEAD_WORD;
				m_nHisWord2 = pWordSeq[0];//HEAD_WORD;//
			}
		}
	}
//	else //高升编辑
//		printf("No sentence was recognised!, please check your program codes or your input data!\n");

	return;
}

void GetPartialFirmwords()
{
	char chtmp[30] = "";
	/*
	for(int j = 0; j < MAX_WORD_LENGTH * 2; j++)
		chtmp[j] = g_stHanziString[g_pWordLattice->m_partialWord * MAX_WORD_LENGTH * 2 + j];
	strcpy(m_firmWords, chtmp);
	*/
	strcpy(m_firmWords, "");
	for(int i = 0; i < g_pWordLattice->m_partialNum; i++)
	{
		for(int j = 0; j < MAX_WORD_LENGTH * 2; j++)
			chtmp[j] = g_stHanziString[g_pModeManager->GetActiveMode()][g_pWordLattice->m_partialWord[i] * MAX_WORD_LENGTH * 2 + j];
		strcat(m_firmWords, chtmp);
	}
}

//下面这个函数是装入此点数组，这个数组用于将来的句子输出，他记载了各个词的号
//wsj 2000.7.18
int LoadHanziData(char *stHanziFileName, char *stHanziString, short mode)
{
	int i, j;
	FILE *pFileName;
	char str[MAX_WORD_LENGTH * 2 + 1];

	if((pFileName = fopen(stHanziFileName, "rt")) == NULL){
		printf("dictionary not found\n");
		return 1;
	}	
	
	long offset = 0;

	int totalWordNum;
	if(mode == DICTATION_MODE)
		totalWordNum = NO_WORDS;
	else
		fscanf(pFileName, "%d", &totalWordNum);

	for(i = 0; i < totalWordNum; i++) {
		fscanf(pFileName, "%s", str);	
		for(j = 0; j < MAX_WORD_LENGTH * 2; j++)
			stHanziString[offset + j] = str[j];
		offset += MAX_WORD_LENGTH * 2;	
	}
	
	fclose(pFileName);

	return 0;
}

//RECOGNIZE_API int SrStartDragonVoice(char* modelType)
int SrStartDragonVoice(char* modelType)
{
	//变量初始化
	m_nHisWord1 = HEAD_WORD;
	m_nHisWord2 = HEAD_WORD; //高升添加
	eventCounter = 0;
	g_pAM = NULL;
	g_pLM = NULL;
	g_pLextree = NULL;
	g_pWordLattice = NULL;
	g_pPath = NULL;
	g_pAudio = NULL;
	g_hEvent = NULL;
	g_fTerminate = FALSE;
	g_RecogThread = NULL;
	g_stimulateThread=NULL;
	g_hMsgTarget = NULL;
	timeAxis = 0;
	//
	g_pModeManager = new CModeManager;

	//初始化各个类
	strcpy(amTopic, "AM ");
	strcat(amTopic, modelType);
	//
	g_pAM = new CAM;
	g_pAM->Init( ); //装载声学有关的数据

	g_pLM = new CLM;
	g_pLM->Init( ); //初始化

	g_pPath = new CPath;
	g_pPath->Init( ); //初始化路径管理中一些变量

	g_pLextree = new CLextree;
	g_pLextree->Init(g_pLM, g_pAM); //初始化变量

	g_pWordLattice = new CWordLattice;
	g_pWordLattice->Init(g_pLM);

	g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	//Search

	DWORD threadID;
//	g_RecogThread = CreateThread(NULL, 0, RecogThreadProc, NULL, 0, NULL);
//	g_RecogThread = CreateThread(NULL, 0, RecogThreadProc, NULL, 0, &threadID);
	
	//创建 Audio 对象
	g_pAudio = new CAudioManage; //
	g_pAudio->LoadData();
	//	
	g_stHanziString[0] = new char[NO_WORDS * MAX_WORD_LENGTH * 2];
	for(int i = 1; i < MAX_MODE_NUM; i++)
		g_stHanziString[i] = new char[MAX_USER_DIC * MAX_WORD_LENGTH * 2];
	return	0;
}

//RECOGNIZE_API int SrExitDragonVoice(void)
int SrExitDragonVoice(void)
{
	g_fTerminate = TRUE;
//	SetEvent(g_hEvent);
	//
//	WaitForSingleObject(g_RecogThread, INFINITE);

	CloseHandle(g_hEvent);
	CloseHandle(g_RecogThread);
	CloseHandle(g_stimulateThread);
//
	if (g_pAM != NULL)
		delete g_pAM;
	if (g_pLM != NULL)
		delete g_pLM;

	if (g_pPath != NULL)
		delete g_pPath;
	if (g_pLextree != NULL)
		delete g_pLextree;
	if (g_pWordLattice != NULL)
		delete g_pWordLattice;	

	if (g_pAudio != NULL)
		delete g_pAudio;
	
	if(g_pModeManager != NULL)
		delete g_pModeManager;

	for(int i = 0; i < MAX_MODE_NUM; i++)
		delete g_stHanziString[i];
	return	0;
}


//RECOGNIZE_API int SrMicOn(void)
int SrMicOn(void)
{
	//
/*	int i, loadedModeCt;

	if(SystemExit)
		return 0;

	loadedModeCt = 0;
	for(i = 0; i < MODE_NUM; i++)
		loadedModeCt+=modeManager.TaskSetNum[i];
		//if(!modeManager.isModeLoaded[i])
		//	loadedModeCt++;
	//if(loadedModeCt >= MODE_NUM)
	//	return 1; //not load any mode. need to load mode
	if(loadedModeCt==0)
		return 1;

	if(pDictation->m_Sample.audioState == 0 || pDictation->m_Sample.audioState == 1)
	{
		if(pDictation->m_Sample.audioState == 1)
			if(!pDictation->m_Sample.InitAudioDevice())
				return 2; //error when allocate record device 

		pDictation->ResumeRecord();
		pDictation->m_Sample.audioState = 2;
	}
*/
	if (g_pAudio->audioState == 0 || g_pAudio->audioState == 1)
	{	if (g_pAudio->audioState == 1)
			if (!g_pAudio->InitAudioDevice())
				return 2;		//error when allocate record device

		g_pAudio->ResumeRecord();
		g_pAudio->audioState = 2;
	}
	return 0;
}

//RECOGNIZE_API int SrMicOff(void) //same as srreleaseaudio, maybe not need
int SrMicOff(void) //same as srreleaseaudio, maybe not need
{
/*	if(SystemExit)
		return 0;

	//if(pDictation->m_Sample.audioState == 0 || pDictation->m_Sample.audioState == 2)
	{
		pDictation->m_Sample.CloseRecord();
		pDictation->m_Sample.audioState = 1;
		return 0;
	}
*/

	if (g_pAudio->audioState == 0 || g_pAudio->audioState == 2){
		g_pAudio->CloseRecord();
		g_pAudio->audioState = 1;
		return 0;
	}

	return 1;
}


//RECOGNIZE_API void SrGetFirmWords(char *firmRes)
void SrGetFirmWords(char *firmRes)
{
	strcpy(firmRes, m_firmWords);
	strcpy(m_firmWords, "");
	return;
}

/*
RECOGNIZE_API void SrGetPartialFirmWords(char *firmRes)
{
	char chtmp[30] = "";
	strcpy(firmRes, "");
	for(int i = 0; i < g_pWordLattice->m_partialNum; i++)
	{
		for(int j = 0; j < MAX_WORD_LENGTH * 2; j++)
			chtmp[j] = g_stHanziString[g_pModeManager->GetActiveMode()][g_pWordLattice->m_partialWord[i] * MAX_WORD_LENGTH * 2 + j];
		strcat(firmRes, chtmp);
	}

}
*/

//RECOGNIZE_API void SrSetMsgTarget(HWND  hwnd)
void SrSetMsgTarget(HWND  hwnd)
{
	//pDictation->handleMsg = hwnd;
	g_hMsgTarget = hwnd;
}

//RECOGNIZE_API int SrPauseAudio(void)
int SrPauseAudio(void)
{
/*	if(pDictation->m_Sample.audioState == 2)
	{
		pDictation->m_Sample.SuspendRecord();
		pDictation->m_Sample.audioState = 0;
		return 0;
	}
	else if(pDictation->m_Sample.audioState == 1)
		return 0;
	else
		return 1;
*/
	if (g_pAudio->audioState == 2){
		g_pAudio->SuspendRecord();
		g_pAudio->audioState = 0;
		return 0;
	}
	else if (g_pAudio->audioState == 1)
		return 0;
	else 
		return 1;
}

//RECOGNIZE_API int SrResumeAudio(void)
int SrResumeAudio(void)
{
/*	if(pDictation->m_Sample.audioState == 0)
	{
		pDictation->m_Sample.ResumeRecord();
		pDictation->m_Sample.audioState = 2;
		return 0;
	}
	else if(pDictation->m_Sample.audioState == 2)
		return 0;
	else
		return 1;
*/
	if (g_pAudio->audioState == 0){
		g_pAudio->ResumeRecord();
		g_pAudio->audioState = 2;
		return 0;
	}
	else if (g_pAudio->audioState == 2)
		return 0;
	else
		return 1;
}

//RECOGNIZE_API int SrGetAudioState(void)
int SrGetAudioState(void)
{
/* 	if(SystemExit)
		return 1;

	return	pDictation->m_Sample.audioState;
*/
	return g_pAudio->audioState;
}


//RECOGNIZE_API int SrOfflineCepTestOld(char* dataFile)		// The data file is the cepstrum based
int SrOfflineCepTestOld(char* dataFile)		// The data file is the cepstrum based
{
	FILE *fp;
	int     i;
	int s_l;
	char CepFile[100];//,DataFile[100];	//zj deleted useless  5.12
	short srcData[WAVEBUFSIZE];

	strcpy(CepFile,dataFile);
	if((fp = fopen(CepFile, "rb")) == NULL)
	{
		fclose(fp);
		return 1;
	}

	fseek(fp,0L,SEEK_END);
	s_l = ftell(fp) / sizeof(short);
	int blockNo = s_l / WAVEBUFSIZE;

	fseek(fp,0,SEEK_SET);

	for(i = 0; i < blockNo; i++) //zjnotice: maybe some errors will occur here!
	{   
		if(i == blockNo - 1)
		{
			g_pAudio->speechEnd = 1;
			g_pAudio->m_longSilence = 1;
		}

		fread(srcData,  sizeof(short), WAVEBUFSIZE, fp);
		g_pAudio->WriteFIFO(srcData);
	}
	fclose(fp);
	g_pAudio->WriteFIFO(NULL);

	return 0;
}


//RECOGNIZE_API int SrOfflineCepTest(char* dataFile)		// The data file is the cepstrum based
int SrOfflineCepTest(char* dataFile)		// The data file is the cepstrum based
{
	FILE *fp, *fidx;
	int     i;
	int s_l;
	char CepFile[100];//,DataFile[100];	//zj deleted useless  5.12
	short srcData[WAVEBUFSIZE];

	strcpy(CepFile,dataFile);
	strcat(CepFile, ".dat");
	if((fp = fopen(CepFile, "rb")) == NULL)
	{
		fclose(fp);
		return 1;
	}
	strcpy(CepFile,dataFile);
	strcat(CepFile, ".idx");
	if((fidx = fopen(CepFile, "rb")) == NULL)
	{
		fclose(fidx);
		return 1;
	}

	fseek(fp,0L,SEEK_END);
	s_l = ftell(fp) / sizeof(short);
	int blockNo = s_l / WAVEBUFSIZE;
	//
	fseek(fidx, 0L, SEEK_END);
	blockNo = ftell(fidx);
//	if(ftell(fidx) != blockNo + 1)
//		return 1;
	//
	fseek(fidx, 0L, SEEK_SET);
	fseek(fp,0,SEEK_SET);
	for(i = 0; i < blockNo; i++)
	{ 
		g_pAudio->speechEnd = 0;
		fread(srcData,  sizeof(short), WAVEBUFSIZE, fp);
		g_pAudio->WriteFIFO(srcData);
		//
		fread(&g_pAudio->m_longSilence, sizeof(char), 1, fidx);
		if(g_pAudio->m_longSilence)
		{
			g_pAudio->speechEnd = 1;
			g_pAudio->m_longSilence = 1;
			g_pAudio->WriteFIFO(NULL);
		}
		totalFrame+=FRAMESPERBUF;
	}

	fclose(fp);
	fclose(fidx);
	//g_pAudio->WriteFIFO(NULL);
	return 0;
}

/*
int SrOfflineCepTest(char* dataFile)		// The data file is the cepstrum based
{
	FILE *fp, *fidx;
	int     i;
	int s_l;
	char CepFile[100];//,DataFile[100];	//zj deleted useless  5.12
	short srcData[WAVEBUFSIZE];

	strcpy(CepFile,dataFile);
	//strcat(CepFile, ".dat");
	if((fp = fopen(CepFile, "rb")) == NULL)
	{
		fclose(fp);
		return 1;
	}

	fseek(fp,0L,SEEK_END);
	s_l = ftell(fp) / sizeof(short);
	int blockNo = s_l / WAVEBUFSIZE;
	//
	fseek(fp,0,SEEK_SET);
	for(i = 0; i <= blockNo; i++)
	{ 
		g_pAudio->speechEnd = 0;
		if(i == blockNo)
		{
			g_pAudio->speechEnd = 1;
			g_pAudio->m_longSilence = 1;
			g_pAudio->WriteFIFO(NULL);
		}
		else
		{
			fread(srcData,  sizeof(short), WAVEBUFSIZE, fp);
			g_pAudio->WriteFIFO(srcData);
			totalFrame+=FRAMESPERBUF;
		}
	}

	fclose(fp);
	//g_pAudio->WriteFIFO(NULL);
	return 0;
}
*/
//RECOGNIZE_API void SrOfflineStimulate( )		// The data file is the cepstrum based
void SrOfflineStimulate( )		// The data file is the cepstrum based
{
	if(g_stimulateThread)
		CloseHandle(g_stimulateThread);
//	g_stimulateThread=CreateThread(NULL, 0, StimulateThreadProc, NULL, 0, NULL);
	DWORD threadID;
	g_stimulateThread=CreateThread(NULL, 0, StimulateThreadProc, NULL, 0, &threadID);
}

void ModifyData(short mode)
{
	g_pLextree->m_lpLextree = g_pLextree->m_lpLextreeMode[mode];
	g_pLM->m_pUnigram = g_pLM->m_pUnigramMode[mode];
	g_pLM->m_pBigram = g_pLM->m_pBigramMode[mode];
	g_pLM->m_pBigramIdx = g_pLM->m_pBigramIdxMode[mode];
	g_pLM->m_pTrigram = g_pLM->m_pTrigramMode[mode];
	g_pLM->m_pTrigramIdx[0] = g_pLM->m_pTrigramIdxMode[mode][0];
	g_pLM->m_pTrigramIdx[1] = g_pLM->m_pTrigramIdxMode[mode][1];
	//
#ifdef USE_INDEX_COMPRESS
	g_pLM->m_pTrigramIdxOffset = g_pLM->m_pTrigramIdxOffsetMode[mode];
#endif

}

int LoadMode(short mode)
{
	int rett=-1;

	if(!g_pModeManager->CheckModeLoaded(mode))
		return 0; //已经装载

	char stHanziFileName[MAX_PATH];
	DWORD dwSize;
	switch(mode)
	{
		case DICTATION_MODE:
			g_pLextree->LoadLextree(mode); //初始化变量和装载词树
	
			rett = g_pLM->LoadData(mode);
			if(rett)
				return rett;//3; //语言模型的相关文件找不到

			dwSize = GetPrivateProfileString("LM", "HanziDictation", NULL, stHanziFileName,
				MAX_PATH, "SearchEngine.ini");
			ASSERT(dwSize != 0);
			if(LoadHanziData(stHanziFileName, g_stHanziString[mode], mode))
				return 2; //汉字文件找不到
			//设置模式
			g_pModeManager->SetActiveMode(mode);
			g_pModeManager->SetLoadedMode(mode);
			//更改数据指针:词树，语言模型，汉字
			ModifyData(mode);
			//
			OnepassSearchEngineInit(); //高声编辑
			break;
		case COMMAND_MODE:
			g_pLextree->LoadLextree(mode); //初始化变量和装载词树

			dwSize = GetPrivateProfileString("LM", "HanziCommand", NULL, stHanziFileName,
				MAX_PATH, "SearchEngine.ini");
			ASSERT(dwSize != 0);
			if(LoadHanziData(stHanziFileName, g_stHanziString[mode], mode))
				return 2; //汉字文件找不到
			//设置模式
			g_pModeManager->SetActiveMode(mode);
			g_pModeManager->SetLoadedMode(mode);
			//
			ModifyData(mode);
			//
			OnepassSearchEngineInit(); //高声编辑
			break;
		case ADAPT_MODE:
			g_pLextree->LoadLextree(mode); //初始化变量和装载词树
	
			if(g_pLM->LoadData(mode))
				return 3; //语言模型的相关文件找不到

			dwSize = GetPrivateProfileString("LM", "HanziDictationAdapt", NULL, stHanziFileName,
				MAX_PATH, "SearchEngine.ini");
			ASSERT(dwSize != 0);
			if(LoadHanziData(stHanziFileName, g_stHanziString[mode], mode))
				return 2; //汉字文件找不到
			//设置模式
			g_pModeManager->SetActiveMode(mode);
			g_pModeManager->SetLoadedMode(mode);
			//更改数据指针:词树，语言模型，汉字
			ModifyData(mode);
			//
			OnepassSearchEngineInit(); //高声编辑
			break;
		default:
			break;
	}
	return 0;
}

int UnLoadMode(short mode)
{
	if(g_pModeManager->GetActiveMode() == mode)
		return 2; //激活的模式不能卸载

	char stHanziFileName[MAX_PATH];
	switch(mode)
	{
		case DICTATION_MODE:
			g_pLextree->UnLoadLextree(mode); //卸载词树
			g_pLM->UnLoadData(mode); //卸在语言模型
			//复位模式
			g_pModeManager->ResetMode(mode);
			break;
		case COMMAND_MODE:
			g_pLextree->UnLoadLextree(mode); //卸载词树
			//复位模式
			g_pModeManager->ResetMode(mode);
			break;
		default:
			break;
	}
	return 0;
}

//RECOGNIZE_API int  SrLoadDictationMode()
int  SrLoadDictationMode()
{
	int ret;
	SrPauseAudio();
	ret = LoadMode(DICTATION_MODE);
	SrResumeAudio();
	return ret;
}

int  SrUnLoadDictationMode()
//RECOGNIZE_API int  SrUnLoadDictationMode()
{
	SrPauseAudio();
	return UnLoadMode(DICTATION_MODE);
}

//RECOGNIZE_API int  SrLoadCommandMode()
int  SrLoadCommandMode()
{
	int ret;
	SrPauseAudio();
	ret = LoadMode(COMMAND_MODE);
	SrResumeAudio();
	return ret;
}

//RECOGNIZE_API int  SrUnLoadCommandMode()
int  SrUnLoadCommandMode()
{
	SrPauseAudio();
	return UnLoadMode(COMMAND_MODE);
}

//RECOGNIZE_API int SrSetActiveWorkingMode(int mode)
int SrSetActiveWorkingMode(int mode)
{
	SrPauseAudio();
	if(g_pModeManager->CheckModeLoaded(mode))
	{
		SrResumeAudio();
		return 1; //该模式没有装载
	}

	if(g_pModeManager->SetActiveMode(mode))
	{
		SrResumeAudio();
		return 2; //mode值超出
	}
	ModifyData(mode);
	OnepassSearchEngineInit(); //高声编辑

	SrResumeAudio();
	return 0;
}

//RECOGNIZE_API int SrGetActiveWorkingMode()
int SrGetActiveWorkingMode()
{
	return g_pModeManager->GetActiveMode();
}

//RECOGNIZE_API int  SrAllocateAudio(void)
int  SrAllocateAudio(void)
{
	/*
	if(pDictation->m_Sample.audioState == 1)
	{
		if(pDictation->m_Sample.InitAudioDevice())
		{
			pDictation->m_Sample.audioState = 0;
			return 0;
		}
		else
			return 1;
	}
	*/
	if(g_pAudio->audioState == 1)
	{
		if(g_pAudio->InitAudioDevice())
		{
			g_pAudio->audioState = 0;
			return 0;
		}
		else
			return 1;
	}
		return 0;
}

//RECOGNIZE_API int  SrReleaseAudio(void)
int  SrReleaseAudio(void)
{
	/*
	if(pDictation->m_Sample.audioState == 0 || pDictation->m_Sample.audioState == 2)
	{
		pDictation->m_Sample.CloseRecord();
		pDictation->m_Sample.audioState = 1;
	}
	*/
	if(g_pAudio->audioState == 0 || g_pAudio->audioState == 2)
	{
		g_pAudio->CloseRecord();
		g_pAudio->audioState = 1;
	}
	return 0;
}

//RECOGNIZE_API int	SrGetAudioVolume(void)
int	SrGetAudioVolume(void)
{
	short int vol = 0;
	if(!g_fTerminate)
	{
		vol = (g_pAudio->E_now / WAVEBUFSIZE)* 20 / 2000; //range: 0 -- 100 vol = __min(vol,20); return vol;
		if(vol > 20)
			vol = 20;
	}
	return vol;
}

//The following function relates to speaker adaptation
//RECOGNIZE_API int SrStartDragonAdapt(char* modelType)
int SrStartDragonAdapt(char* modelType)
{
	/*/on July 5, 1999
	isStartAdapt = 1; //start adapt function
	//
	pDictation = new CDictation;
	if(!pDictation)
		return	1;

	if(!pDictation->m_Sample.InitAudioDevice())
	{
		delete pDictation;
		return 1;
	}

	if(pDictation->LoadCommonData(modelType))
	{
		delete pDictation;
		return 1;
	}
	pDictation->confidenceThreshold = 0.1 * 6; 
	*/
	//变量初始化
	m_nHisWord1 = HEAD_WORD;
	m_nHisWord2 = HEAD_WORD; //高升添加
	eventCounter = 0;
	g_pAM = NULL;
	g_pLM = NULL;
	g_pLextree = NULL;
	g_pWordLattice = NULL;
	g_pPath = NULL;
	g_pAudio = NULL;
	g_hEvent = NULL;
	g_fTerminate = FALSE;
	g_RecogThread = NULL;
	g_stimulateThread=NULL;
	g_hMsgTarget = NULL;
	timeAxis = 0;
	//
	g_pModeManager = new CModeManager;

	//初始化各个类
	strcpy(amTopic, "AM ");
	strcat(amTopic, modelType);
	//
	g_pAM = new CAM;
	g_pAM->Init( ); //装载声学有关的数据

	g_pLM = new CLM;
	g_pLM->Init( ); //初始化

	g_pPath = new CPath;
	g_pPath->Init( ); //初始化路径管理中一些变量

	g_pLextree = new CLextree;
	g_pLextree->Init(g_pLM, g_pAM); //初始化变量

	g_pWordLattice = new CWordLattice;
	g_pWordLattice->Init(g_pLM);

	g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	//Search


//	g_RecogThread = CreateThread(NULL, 0, RecogThreadProcAdapt, NULL, 0, NULL);
	DWORD threadID;
	g_RecogThread = CreateThread(NULL, 0, RecogThreadProcAdapt, NULL, 0, &threadID);
	
	//创建 Audio 对象
	g_pAudio = new CAudioManage; //
	g_pAudio->LoadData();
	//	
	g_stHanziString[0] = new char[NO_WORDS * MAX_WORD_LENGTH * 2];
	for(int i = 1; i < MAX_MODE_NUM; i++)
		g_stHanziString[i] = new char[MAX_USER_DIC * MAX_WORD_LENGTH * 2];
	LoadMode(DICTATION_MODE);

	return	0;
}

//Same as SrExitDragonAdapt.
//RECOGNIZE_API int SrExitDragonAdapt(void)
int SrExitDragonAdapt(void)
{
/*	if(pDictation->recogState)
		return 1;
	pDictation->exitThreadSign = 1;
	Sleep(500);
	delete	pDictation;

	return	0;
*/
	g_fTerminate = TRUE;
	SetEvent(g_hEvent);
	//
	//WaitForSingleObject(g_RecogThread, INFINITE);

	CloseHandle(g_hEvent);
	CloseHandle(g_RecogThread);
//
	CloseHandle(g_stimulateThread);
//
	if (g_pAM != NULL)
		delete g_pAM;
	if (g_pLM != NULL)
		delete g_pLM;

	if (g_pPath != NULL)
		delete g_pPath;
	if (g_pLextree != NULL)
		delete g_pLextree;
	if (g_pWordLattice != NULL)
		delete g_pWordLattice;	

	if (g_pAudio != NULL)
		delete g_pAudio;
	
	if(g_pModeManager != NULL)
		delete g_pModeManager;

	for(int i = 0; i < MAX_MODE_NUM; i++)
		delete g_stHanziString[i];
	return	0;
}
//start to utter the next sentence
//RECOGNIZE_API int SrStartNextSentence(void)
int SrStartNextSentence(void)
{
	/*/
	pDictation->m_Sample.pHead = pDictation->m_Sample.pTail = pDictation->m_Sample.featureStack;
	pDictation->m_Sample.localPos = 0;
	pDictation->m_Sample.processingFramesNum = 0;
	pDictation->m_Sample.currentBufPos = 0;
	pDictation->m_Sample.checkNoise = FALSE;
	pDictation->m_Sample.noSpeech = 0;
	pDictation->m_Sample.speechEnd = FALSE;
	pDictation->m_Sample.speechBegin = FALSE;
	pDictation->m_Sample.Eavg = 0;
	pDictation->m_Sample.E_now = 0;
	pDictation->m_Sample.firstSpeechBlock = 1;
	/*/
	OnepassSearchEngineInit();
	g_pAudio->processingFramesNum = 0;
	g_pAudio->processingWaveNum = 0;
	if(g_pAudio->audioState == 0 || g_pAudio->audioState == 1)
	{
		if(g_pAudio->audioState == 1)
			if(!g_pAudio->InitAudioDevice())
				return 1; //error when allocate record device 

		g_pAudio->ResumeRecord();
		g_pAudio->audioState = 2;
	}

	return 0;
}
//Parameter: fileName --->file name to store the features.
//If specified file exist, then overwrite it. So developers must check whether
// specified file exists by himself.
//RECOGNIZE_API int SrSaveFeatureAdapt(char *fileName)
int SrSaveFeatureAdapt(char *fileName)
{
	FILE *fp;
	int i;
	short len;

	if((fp = fopen(fileName, "wb")) == NULL)
		return 1; //error when creating file
	len = g_pAudio->processingFramesNum;
	fwrite(&len, sizeof(short), 1, fp);
	fwrite(g_pAudio->featureVector, sizeof(float), NO_DIM*len, fp);
	fclose(fp);
	return 0;
}
//Parameter: fileName --->file name to store the speech wave data.
//If specified file exist, then overwrite it. So developers must check whether
// specified file exists by himself.
//RECOGNIZE_API int SrSaveWaveAdapt(char *fileName)
 int SrSaveWaveAdapt(char *fileName)
{
	FILE *fp;
	if((fp = fopen(fileName, "wb")) == NULL)
		return 1; //error when creating file
	fwrite(g_pAudio->waveData, sizeof(short), g_pAudio->processingWaveNum * HFRAMELEN, fp);
	fclose(fp);
	return 0;
}

/*/Parameter:
RECOGNIZE_API int SrSetPinyinAdapt(char **pinYin, int number)
{
	int i;

	if(number > MAX_PHONE_NUM / 2)
		return 1;//the number of Chinese characters is over the maximum of this system.
	for(i = 0; i < number; i++)
		strcpy(pDictation->truePinYin[i], pinYin[i]);
	pDictation->truePinYinLen = number;
	i = pDictation->m_2rdPass.ConvertHmmModes(pDictation->truePinYin, number, pDictation->truePhoneIdx);
	if(!i)
		return 2;//某个拼音不在拼音表中
	return 0;
}
RECOGNIZE_API int SrGetConfidenceAdapt(void)
{
	float score;
	int  i, len1, len2, trueMap[MAX_PHONE_NUM], resMap[MAX_PHONE_NUM];
	
	len1 = pDictation->truePinYinLen * 2;
	for(i = 0; i < len1; i++)
		trueMap[i] = pDictation->m_FirstPass.mapping[pDictation->truePhoneIdx[i]];
	len2 = pDictation->m_FirstPass.phoneStrLength;
	for(i = 0; i < len2; i++)
		resMap[i] = pDictation->m_FirstPass.mapping[pDictation->m_FirstPass.bestPhoneString[i]];

	if(len2 > 0)
	{
		score = pDictation->m_FirstPass.StringMatch(trueMap, len1, resMap, len2);
		if(score < 0.00)
			return 1; //reject
		else
			return 0; //accept
	}
	else
		return 1;
}

//the following API functions is for adaptation training. July 7, 1999
RECOGNIZE_API int SrInitReestimate(char *HmmFile, char *NewHmmFile, char *ScriptFile, char *SentencePath,
						   int NoSentence)
{
	
//	MessageBox(NULL,"start to test message box!",NULL,MB_OK);
	int len;

	pAdapt = new CAdapt;
	if(!pAdapt)
		return 1; //memory is insufficient. July 7, 1999
	len = strlen(HmmFile);
	pAdapt->HmmFileName = new char[len+1];
	strcpy(pAdapt->HmmFileName, HmmFile);

	len = strlen(NewHmmFile);
	pAdapt->NewHmmFileName = new char[len+1];
	strcpy(pAdapt->NewHmmFileName, NewHmmFile);

	len = strlen(ScriptFile);
	pAdapt->ScriptFileName = new char[len+1];
	strcpy(pAdapt->ScriptFileName, ScriptFile);

	len = strlen(SentencePath);
	pAdapt->Path = new char[len+1];
	strcpy(pAdapt->Path, SentencePath);

	pAdapt->No_Sentence = NoSentence;
	
	pAdapt->m_bBreak=0;

	if(!pAdapt->Load_Hmm())
	{
		delete pAdapt;
		return 2; //model is not exist. July 7, 1999
	}
	
	pAdapt->Data=new AdaptData[pAdapt->No_Sentence];
	if(pAdapt->Data==NULL) 
	{
		delete pAdapt;
		return 1;
	}

	return 0;

}
RECOGNIZE_API int  SrReestimate(HANDLE handle)
{
	pAdapt->hp_ipsi=new _int16[(long)Max_SL*Max_Phone*Max_State];

	if(!pAdapt->Map_Vfs((HWND)handle)) 
	{
		delete pAdapt;
		return 1;
	}

	delete pAdapt->hp_ipsi;
	delete pAdapt->Data;

	if (!pAdapt->m_bBreak)
	{
		pAdapt->Write_Hmm();
		pAdapt->ReverseHmmModel();
	}

	pAdapt->ReleaseHmm();
	return 0;
}

RECOGNIZE_API int  SrAbortReestimate(void)
{
	pAdapt->m_bBreak=1;
	delete pAdapt;
	return 0;
}
*/
//RECOGNIZE_API int SrPlaySound(char* fileName)
int SrPlaySound(char* fileName)
{
	FILE *fp;
	char head[44], *data;
	int len;
	strcpy(head, "RIFF");
	strcpy(head+8, "WAVEfmt ");
	*(int*)(head+16) = 16;
	*(short*)(head+20) = 1;
	*(short*)(head+22) = 1;
	*(int*)(head+24) = 16000;
	*(int*)(head+28) = 32000;
	*(short*)(head+32) = 2;
	*(short*)(head+34) = 16;
	strcpy(head+36, "data");
	
	if((fp = fopen(fileName, "rb")) == NULL)
	{
		return 2;
	}
	fseek(fp, 0L, SEEK_END); 
	len = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	data = new char[len+44];
	if(!data)
	{
		fclose(fp);
		return 3;
	}
	fread(data+44, sizeof(char), len, fp);
	fclose(fp);
	int len1=len+36;
	*(int*)(head+4) = len1;
	*(int*)(head+40) = len;
	memcpy(data,head,44);

	//int ret = PlaySound(fileName, NULL, SND_FILENAME);
	int ret = PlaySound(data, NULL, SND_MEMORY);
	delete data;
	if(ret)
		return 0;
	else
		return 1;
}



//

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

// This is an example of an exported function.
//RECOGNIZE_API int fnRecognize(void)
//{
//	return 42;
//}


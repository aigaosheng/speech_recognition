#include "stdafx.h"

#include "const.h"
#include "global.h"

extern void OnepassSearchEngineInit(void);

//DWORD WINAPI RecogThreadProc(LPVOID lpParam)
int RecogThreadProc()
{

//	while (TRUE){
	//	DWORD dwObject = WaitForSingleObject(g_hEvent, INFINITE);

		//if (!g_fTerminate）

		while (1){
			if(g_pAudio->ReadFIFO() == FIFO_NULL || g_fTerminate)
			{
				break;
			}
		}

	//	if (g_fTerminate) //高生编辑
		//	break;
//	}
	return 0;
}

DWORD WINAPI RecogThreadProcAdapt(LPVOID lpParam)
{

	while (TRUE){
		DWORD dwObject = WaitForSingleObject(g_hEvent, INFINITE);
		//if (!g_fTerminate）
		while (1){
			if(g_pAudio->ReadFifoAdapt() == FIFO_NULL || g_fTerminate)
				break;
		}
		//
		::SendMessage(g_hMsgTarget, WM_DRAGONVOICE_ADAPT_NEXT, 0L, 0L);
		//
		if (g_fTerminate) //高生编辑
			break;
	}
	return 0;
}

DWORD WINAPI StimulateThreadProc(LPVOID lpParam)
{
	FILE *fp, *fidx;
	int     i;
	int s_l;
	char CepFile[100];
	short srcData[WAVEBUFSIZE];

//SuspendThread(g_RecogThread);

	TCHAR lpszFileName[MAX_PATH];
	DWORD dwSize = GetPrivateProfileString("OFFLINE", "TestFile", NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);

	strcpy(CepFile,lpszFileName);
	strcat(CepFile, ".dat");
	if((fp = fopen(CepFile, "rb")) == NULL)
	{
		fclose(fp);
		return 1;
	}
	strcpy(CepFile,lpszFileName);
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
		fread(&g_pAudio->m_longSilence, sizeof(char), 1, fidx);
		if(g_pAudio->m_longSilence)
		{
			g_pAudio->speechEnd = 1;
			g_pAudio->m_longSilence = 1;
			g_pAudio->WriteFIFO(NULL);
		}
		else
		{
			g_pAudio->speechEnd = 0;
			fread(srcData,  sizeof(short), WAVEBUFSIZE, fp);
			g_pAudio->WriteFIFO(srcData);
		}
	}

	fclose(fp);
	fclose(fidx);
	//g_pAudio->WriteFIFO(NULL);
//ResumeThread(g_RecogThread);

	return 0;
}
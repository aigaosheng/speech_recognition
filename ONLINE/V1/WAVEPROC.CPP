/****************************************************************
  FILE NAME: WAVEPROC.CPP

  DESCRIPTION: The function is to record, put the source data
  into a FIFO and store it.

  WRITTEN BY: XuBo
  MODIFIED BY: GaoSheng
  DATE: April 6,1999
****************************************************************/
#include	"waveproc.h"
#include	"const.h"
#include	"global.h"

#include	"audio.h"
#include	"assert.h"

#define MAX_BLOCK_SPEECH_END	1
#define MAX_LONG_SILENCE_BLOCK	3//5//5//3

//FILE *hFile;

void CALLBACK waveInProc(HWAVEIN hWaveIn,UINT uMsg,DWORD dwInstance,DWORD lParam1,DWORD lParam2)
{
	short	*lpData, localWaveBuffer[WAVEBUFSIZE];
	int	i, j, dwBytesRecorded, E[FRAMESPERBUF], dat0;

	if(uMsg == MM_WIM_CLOSE)
	{
/*		if(g_pAudio->pHead != g_pAudio->pTail)
		{
			if(g_pAudio->pHead == g_pAudio->featureStack)
				(g_pAudio->featureStack+ MAXFIFOITEMSIZE - 1)->speechSegment = 2;
			else
				(g_pAudio->pHead-1)->speechSegment = 2;
		}
		g_pAudio->WriteFIFO(NULL);
*/
		return;
	}
	if(uMsg != MM_WIM_DATA)
		return;

	dwBytesRecorded = ((LPWAVEHDR)lParam1)->dwBytesRecorded;
	if(((LPWAVEHDR)lParam1)->dwBytesRecorded != (WAVEBUFSIZE * sizeof(short)))
		return;

	lpData = (short *)((LPWAVEHDR)lParam1)->lpData;
	memcpy(localWaveBuffer, lpData, sizeof(short)*WAVEBUFSIZE);
	waveInUnprepareHeader(hWaveIn, (LPWAVEHDR)lParam1, sizeof(WAVEHDR));

	if(g_pAudio->checkNoise) 
	{
		//减去背景噪声
		for(i = 0; i < WAVEBUFSIZE; i++)
			localWaveBuffer[i] -= g_pAudio->bias;
	}
	else
	{
		//计算背景噪声
		g_pAudio->bias = 0;
		for(i = 0; i < WAVEBUFSIZE; i++)
			g_pAudio->bias += localWaveBuffer[i];
		g_pAudio->bias /= WAVEBUFSIZE;
	}

	//计算整个录音块的总幅度
	g_pAudio->E_now = 0;
	for(i = 0; i < FRAMESPERBUF; i++)
	{
		E[i] = 0L;
		for(j = 0; j < HFRAMELEN; j++)
		{
			dat0 = localWaveBuffer[i*HFRAMELEN+j];
			E[i] += abs(dat0);
		}
		g_pAudio->E_now += E[i];
	}
	//
	PostMessage(g_hMsgTarget, WM_DRAGONVOICE_GETVOLUME, 0L, 0L);
	//
	if(g_pAudio->checkNoise)
	{
		//如果已经检测过背景噪声
		if(!g_pAudio->speechBegin) //initialized value is 0
		{
			//如果语音没有开始，就检测语音的开始点
			if(g_pAudio->E_now > g_pAudio->E_start) 
			{
				//如果当前数据块的总幅度超过语音开始门限，则标记语音开始
				g_pAudio->speechBegin = TRUE; //speech data begins
				g_pAudio->Eavg = g_pAudio->E_now;
			}
			else
			{
				//如果当前数据块的幅度小于语音开始门限，则语音没有开始，用当前数据块的幅度更新语音开始和结束的门限
				g_pAudio->E_start = MAX_BEGINE_AMPLITUDE * g_pAudio->E_now; //dramatically modify the threshold of speech start
				g_pAudio->E_end = MAX_END_AMPLITUDE * g_pAudio->E_now;//dramatically modify the threshold of speech end
			}
		}
		else
		{
			//如果语音已经开始，则检测当前数据是否语音结束点
			if(g_pAudio->E_now > g_pAudio->E_end)
			    g_pAudio->noSpeech = 0; 
			else
				g_pAudio->noSpeech++; //如果当前块的幅度小于语音结束门限，则记录语音结束
		
			if(g_pAudio->noSpeech >= MAX_BLOCK_SPEECH_END)
			{
				//如果连续N块数据都小于语音结束门限，则标记语音的结束
				g_pAudio->speechEnd = TRUE;	
				//
//				SetEvent(g_hEvent);
//				eventCounter = InterlockedIncrement(&eventCounter); // 高生编辑
			}
			g_pAudio->Eavg += g_pAudio->E_now;
		}
	}
	else
    {
		//如果录音设备刚开始启动，计算语音开始和结束的阀值
		g_pAudio->E_start = MAX_BEGINE_AMPLITUDE * g_pAudio->E_now;
		g_pAudio->E_end = MAX_END_AMPLITUDE * g_pAudio->E_now;
		g_pAudio->checkNoise = TRUE;
	}
	//释放录音所占用的buffer
	if(!g_pAudio->speechBegin)
	{
		//语音没有开始，则丢掉该块
		int ret = waveInPrepareHeader((HWAVEIN)hWaveIn, (LPWAVEHDR)lParam1, sizeof(WAVEHDR));
		if(!ret)
			waveInAddBuffer((HWAVEIN)hWaveIn, (LPWAVEHDR)lParam1, sizeof(WAVEHDR));
		//统计自从上次语音结束后，静音块的个数
		if(g_pAudio->m_preSpeechState)
		{
			g_pAudio->m_silenceBlockCount++;
			if(g_pAudio->m_silenceBlockCount >= MAX_LONG_SILENCE_BLOCK)
			{
				//长时停顿
				g_pAudio->WriteFIFO(NULL);
				g_pAudio->m_silenceBlockCount = 0;
				g_pAudio->m_preSpeechState = 0;
			}

		}
		return;
	}
	//语音开始
	g_pAudio->currentBufPos += WAVEBUFSIZE;
	if(g_pAudio->speechEnd)
	{
		//语音结束
		g_pAudio->m_preSpeechState = g_pAudio->speechBegin;
		g_pAudio->m_silenceBlockCount = 0;
		//
		int  frameNo = g_pAudio->currentBufPos / HFRAMELEN;
		// 计算幅度长时点平均值
		g_pAudio->Ew = (float)g_pAudio->Eavg * HFRAMELEN / (float)(g_pAudio->currentBufPos);
		//计算开始门限的点估计
		g_pAudio->Es = (float)g_pAudio->E_start / FRAMESPERBUF;
		// Verify the Speech Detection:Duration & Average Energy
		if(frameNo <= MINSPEECHLENGTH || g_pAudio->Ew <= 1 * g_pAudio->Es)
		{
			g_pAudio->m_silenceBlockCount++;
			//
			g_pAudio->speechEnd = 0;
			g_pAudio->speechBegin = 0;
			g_pAudio->noSpeech = 0;
			g_pAudio->currentBufPos = 0;
			g_pAudio->Eavg = 0;
			if(waveInPrepareHeader((HWAVEIN)hWaveIn, (LPWAVEHDR)lParam1, sizeof(WAVEHDR)))
				return;
			else
				waveInAddBuffer((HWAVEIN)hWaveIn, (LPWAVEHDR)lParam1, sizeof(WAVEHDR));
			return;
		}	
		g_pAudio->m_silenceBlockCount++;
		//
		g_pAudio->noSpeech = 0;
		g_pAudio->currentBufPos = 0;
		g_pAudio->Eavg = 0;
		g_pAudio->speechBegin = 0;
	}
	//把录音数据写入FIFO
	g_pAudio->WriteFIFO((short*)localWaveBuffer);	
	//
	if(waveInPrepareHeader((HWAVEIN)hWaveIn, (LPWAVEHDR)lParam1, sizeof(WAVEHDR)))
		return;
	else
	{
		if(waveInAddBuffer((HWAVEIN)hWaveIn, (LPWAVEHDR)lParam1, sizeof(WAVEHDR)))
			return;
	}
	return;
}
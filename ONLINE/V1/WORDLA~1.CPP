// m_WordLattice.cpp: implementation of the CWordLattice class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "const.h"
#include "global.h"

#define _DEBUG
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWordLattice::CWordLattice()
{
	m_nLastTime = 0;
	m_nSeqInc = 0;
	//memset(m_WordLattice,0,MAX_SPEECH_LENGTH*MAX_WORD_CANDIDATES*sizeof(LATTICENODE));
	memset(m_WordLattice,0,MAX_WORD_LATTICE_STACK*MAX_WORD_CANDIDATES*sizeof(LATTICENODE));
	//一下由高声编辑
	m_FreeLatticeStackTop = 0;
	m_traceCount = 0;
	for(int i = 0; i < MAX_CONTINUOUS_TIME; i++)
	{
	//	m_FreeLatticeStack[i] = i; //空闲lattice入栈
		m_TimeLatticeMap[i] = -1; //时间和lattice位置映射复位
	}
	for(i = 0; i < MAX_WORD_LATTICE_STACK; i++)
		m_FreeLatticeStack[i] = i; //空闲lattice入栈
	m_refAmScore = 0;
	m_refLmScore = 0;

}

CWordLattice::~CWordLattice()
{
}

void CWordLattice::Init(CLM *lm)
{
	m_LM = lm;
}

void CWordLattice::ResetWordCounter()
{
	ZeroMemory((PVOID)m_ValidNum,(DWORD)MAX_WORD_LATTICE_STACK*sizeof(int));
	m_nLastTime = 0;
//	m_nSeqInc = 0;
	m_PreConfirmTime = -1;
	g_pWordLattice->m_traceStartTime = MIN_TRACE_START_TIME;
}

void CWordLattice::PushLattice(int nTime,int nWordNumber,LPLATTICENODE pLattice)
{
/*
FILE *fp=fopen("dbgLattice1.txt", "a+t");
fprintf(fp, "%d, %d\n", nTime, nWordNumber);
for(int i=0;i<nWordNumber;i++)
{
	fprintf(fp, "w=%-6d,h1=%-6d,h2=%-6d,t=%-4d,p=%-3d,am=%d, lm=%d, sum=%d\n", pLattice[i].wordId, pLattice[i].hisWord1, pLattice[i].hisWord2, 
		pLattice[i].preTime, pLattice[i].preWordPos, pLattice[i].amScore,pLattice[i].lmScore,pLattice[i].amScore+10* pLattice[i].lmScore);
}
fclose(fp);
*/
	m_nLastTime = nTime;

	//以下由高声编辑
	short freeIdx = m_FreeLatticeStack[m_FreeLatticeStackTop];
	ASSERT(freeIdx < MAX_WORD_LATTICE_STACK);
	m_FreeLatticeStack[m_FreeLatticeStackTop] = -1;
	m_TimeLatticeMap[nTime] = freeIdx;
	ASSERT(nTime < MAX_CONTINUOUS_TIME);
	m_FreeLatticeStackTop++;
	ASSERT(m_FreeLatticeStackTop <= MAX_WORD_LATTICE_STACK);
	memcpy((void *)m_WordLattice[freeIdx],(const void *)pLattice,
		   sizeof(LATTICENODE)*nWordNumber);
	m_ValidNum[freeIdx] = nWordNumber;	
	//
//	memcpy((void *)m_WordLattice[nTime],(const void *)pLattice,
//		   sizeof(LATTICENODE)*nWordNumber);
//	m_ValidNum[nTime] = nWordNumber;	
}

void CWordLattice::GetWordSequence(int &nWordLength,int * WordSquence)
{
	TraceBack();
	nWordLength = m_nSeqInc;
	memcpy((void *)WordSquence,(const void *)m_WordSeq,m_nSeqInc*sizeof(int));
}

void CWordLattice::TraceBack()
{
	int nBestIdx=0;
	int nBackTime=0;
	int nBackPos=0;
	long lProbMax,lProbTmp,lTailLMScore;
	WORD wWord1,wWord2; 
	short timeLatticeMap, tt;

	timeLatticeMap = m_TimeLatticeMap[m_nLastTime];

	for(int i = 0; i < m_ValidNum[timeLatticeMap] ; i++)
	{
		wWord1 = m_WordLattice[timeLatticeMap][i].hisWord2;
		wWord2 = m_WordLattice[timeLatticeMap][i].wordId;

		long lProbBackoff = m_LM->CheckTriBackoff(wWord1, wWord2);
		lTailLMScore = m_LM->GetLMScore(wWord1,wWord2,TAIL_WORD,lProbBackoff);
		m_WordLattice[timeLatticeMap][i].lmScore += lTailLMScore;
	}

	for(tt = m_nLastTime; tt > m_PreConfirmTime; tt--)
	{
		lProbMax = NEG_HUGE;
		nBestIdx = -1;
		timeLatticeMap = m_TimeLatticeMap[tt];
		if(m_ValidNum[timeLatticeMap] > 0)
		{
			for(int i = 0; i < m_ValidNum[timeLatticeMap]; i++)
			{
				lProbTmp = m_WordLattice[timeLatticeMap][i].amScore * ACOUSTIC_WEIGHT 
					+ LANGUAGE_WEIGHT * m_WordLattice[timeLatticeMap][i].lmScore;
				if(lProbTmp > lProbMax)
				{
					lProbMax = lProbTmp;
					nBestIdx = i;
				}
			}
			nBackTime = tt;
			break;
		}
	}

	if(nBestIdx == -1)
	{
		m_nSeqInc = 0;
		return;
	}

	m_nSeqInc = 0;
	timeLatticeMap = m_TimeLatticeMap[nBackTime];
	m_WordSeq[m_nSeqInc] = m_WordLattice[timeLatticeMap][nBestIdx].wordId;
	m_nSeqInc++;
	nBackPos = m_WordLattice[timeLatticeMap][nBestIdx].preWordPos;
	nBackTime = m_WordLattice[timeLatticeMap][nBestIdx].preTime;

	while(nBackTime > m_PreConfirmTime)
	{
		timeLatticeMap = m_TimeLatticeMap[nBackTime];
		m_WordSeq[m_nSeqInc] = m_WordLattice[timeLatticeMap][nBackPos].wordId;
		//
		nBackTime = m_WordLattice[timeLatticeMap][nBackPos].preTime;
		nBackPos = m_WordLattice[timeLatticeMap][nBackPos].preWordPos;
		//
		m_nSeqInc++;
	}
	//释放wordlattice空间
	ReleaseWordLattice(m_PreConfirmTime, m_nLastTime);	
}

//以下有高升编辑，用于incremental的搜索
int CWordLattice::PartialTraceBack(int preTraceTime)
{
	int nBestIdx=0;
	int nBackTime=0;
	int nBackPos=0;
	long lProbMax,lProbTmp,lTailLMScore;
	WORD wWord1,wWord2; 
	int traceTime, tt;
	short timeLatticeMap;
	
	timeLatticeMap = m_TimeLatticeMap[m_nLastTime];
	//找出
	for(tt = m_nLastTime; tt >= 0; tt--)
	{
		lProbMax = NEG_HUGE;
		nBestIdx = -1;
		timeLatticeMap = m_TimeLatticeMap[tt];
		if(m_ValidNum[timeLatticeMap] > 0)
		{
			for(int i = 0; i < m_ValidNum[timeLatticeMap]; i++)
			{
				lProbTmp = m_WordLattice[timeLatticeMap][i].amScore * ACOUSTIC_WEIGHT 
					+ LANGUAGE_WEIGHT * m_WordLattice[timeLatticeMap][i].lmScore;
				if(lProbTmp > lProbMax)
				{
					lProbMax = lProbTmp;
					nBestIdx = i;
				}
			}
			nBackTime = tt;
			break;
		}
	}
	if(nBestIdx == -1)
	  	return preTraceTime;

	traceTime = nBackTime;
	m_nSeqInc = 0;
	timeLatticeMap = m_TimeLatticeMap[nBackTime];
	m_partialResult[m_traceCount].m_wordSeq[m_nSeqInc] = m_WordLattice[timeLatticeMap][nBestIdx].wordId;
	m_partialResult[m_traceCount].m_timeSeq[m_nSeqInc] = nBackTime;
	m_partialResult[m_traceCount].m_scoreAm[m_nSeqInc] = m_WordLattice[timeLatticeMap][nBestIdx].amScore;
	m_partialResult[m_traceCount].m_scoreLm[m_nSeqInc] = m_WordLattice[timeLatticeMap][nBestIdx].lmScore;
	//
	m_nSeqInc++;
	nBackPos = m_WordLattice[timeLatticeMap][nBestIdx].preWordPos;
	nBackTime = m_WordLattice[timeLatticeMap][nBestIdx].preTime;

	while(nBackTime > m_PreConfirmTime)
	{
		timeLatticeMap = m_TimeLatticeMap[nBackTime];
		m_partialResult[m_traceCount].m_wordSeq[m_nSeqInc] = m_WordLattice[timeLatticeMap][nBackPos].wordId;
		m_partialResult[m_traceCount].m_timeSeq[m_nSeqInc] = nBackTime;
		m_partialResult[m_traceCount].m_scoreAm[m_nSeqInc] = m_WordLattice[timeLatticeMap][nBestIdx].amScore;
		m_partialResult[m_traceCount].m_scoreLm[m_nSeqInc] = m_WordLattice[timeLatticeMap][nBestIdx].lmScore;
		//
		nBackTime = m_WordLattice[timeLatticeMap][nBackPos].preTime;
		nBackPos = m_WordLattice[timeLatticeMap][nBackPos].preWordPos;

		m_nSeqInc++;
	}
	//
	//
	if(m_nSeqInc >= 3)
	{
		m_partialResult[m_traceCount].m_nSeqLen = m_nSeqInc;
		m_traceCount++;
	}
	else
	{
		m_partialResult[m_traceCount].m_nSeqLen = 0;
		traceTime = preTraceTime;
	}
	return traceTime;
}

void CWordLattice::	ReleaseWordLattice(int stTime, int endTime)
{
	for(int t = stTime + 1; t <= endTime; t++)
	{
		int timeLatticeMap = m_TimeLatticeMap[t];
		ASSERT(timeLatticeMap < MAX_WORD_LATTICE_STACK);
		if(timeLatticeMap >= 0)
		{
			m_TimeLatticeMap[t] = -1;
			m_FreeLatticeStackTop--;
			ASSERT(m_FreeLatticeStackTop >= 0);
			m_FreeLatticeStack[m_FreeLatticeStackTop] = timeLatticeMap;
		}
		m_ValidNum[timeLatticeMap] = 0;
	}
}

/*
int CWordLattice::GeneratePartialWord()
{
	unsigned short firstWordID;
	int firstWordTime, sameCt, i, j, count, len;
	long refAm;
	long refLm;

	len = m_partialResult[0].m_nSeqLen - 1;
	firstWordID = m_partialResult[0].m_wordSeq[len];
	firstWordTime = m_partialResult[0].m_timeSeq[len];
	refAm = m_partialResult[0].m_scoreAm[len];
	refLm = m_partialResult[0].m_scoreLm[len];
	//
	sameCt = 1;
	for(i = 1; i < MAX_CONFIRM_BLOCK; i++)
	{
		len = m_partialResult[i].m_nSeqLen - 1;
		if(m_partialResult[i].m_wordSeq[len] == firstWordID && m_partialResult[i].m_timeSeq[len] == firstWordTime)
			sameCt++;
	}
	if(sameCt == MAX_CONFIRM_BLOCK)
	{
		ReleaseWordLattice(m_PreConfirmTime, firstWordTime);
		//第一个词是相同，输出
		m_partialWord = firstWordID;
		m_PreConfirmTime = firstWordTime;
		m_refAmScore = refAm / 4;
		m_refLmScore = refLm / 4;
		//
		count = 0;
		for(i = 1; i < MAX_CONFIRM_BLOCK; i++)
		{
			if(m_partialResult[i].m_nSeqLen >= 4)
			{
				memcpy(m_partialResult+count, m_partialResult+i, sizeof(S_PARTIAL_RESULT));
				m_partialResult[count].m_nSeqLen--;
				count++;
			}
		}
		m_traceCount = count;
		return 1;
	}
	else
	{
		//第一个词不同，把第一次的结果移出
		for(i = 1; i < MAX_CONFIRM_BLOCK; i++)
			memcpy(m_partialResult+i-1, m_partialResult+i, sizeof(S_PARTIAL_RESULT));
		m_traceCount--;
		return 0;
	}
}
*/
int CWordLattice::GeneratePartialWord()
{
	unsigned short firstWordID[2];
	int firstWordTime[2], sameCt[2], i, j, count, len;
	long refAm;
	long refLm;

	len = m_partialResult[0].m_nSeqLen - 1;
	for(i = 0; i < 2; i++)
	{
		firstWordID[i] = m_partialResult[0].m_wordSeq[len-i];
		firstWordTime[i] = m_partialResult[0].m_timeSeq[len-i];
		sameCt[i] = 1;
	}
//	refAm = m_partialResult[0].m_scoreAm[len];
//	refLm = m_partialResult[0].m_scoreLm[len];
	//
	for(i = 1; i < MAX_CONFIRM_BLOCK; i++)
	{
		len = m_partialResult[i].m_nSeqLen - 1;
		if(m_partialResult[i].m_wordSeq[len] == firstWordID[0] && m_partialResult[i].m_timeSeq[len] == firstWordTime[0])
		{
			sameCt[0]++;
			if(m_partialResult[i].m_wordSeq[len-1] == firstWordID[1] && m_partialResult[i].m_timeSeq[len-1] == firstWordTime[1])
				sameCt[1]++;
		}
	}
	if(sameCt[0] == MAX_CONFIRM_BLOCK)
	{
		//第一个词是相同，输出
		len = m_partialResult[0].m_nSeqLen - 1;
		m_partialWord[0] = firstWordID[0];
		//
		if(sameCt[1] == MAX_CONFIRM_BLOCK)
		{
			ReleaseWordLattice(m_PreConfirmTime, firstWordTime[1]);
			//第二个词是相同，输出
			m_partialNum = 2;
			m_partialWord[1] = firstWordID[1];
			m_PreConfirmTime = firstWordTime[1];
			m_refAmScore = m_partialResult[0].m_scoreAm[len-1] / 4;
			m_refLmScore = m_partialResult[0].m_scoreLm[len-1] / 4;
			//
			count = 0;
			for(i = 1; i < MAX_CONFIRM_BLOCK; i++)
			{
				if(m_partialResult[i].m_nSeqLen >= 5)
				{
					memcpy(m_partialResult+count, m_partialResult+i, sizeof(S_PARTIAL_RESULT));
					m_partialResult[count].m_nSeqLen -= 2;
					count++;
				}
			}
		}
		else
		{
			ReleaseWordLattice(m_PreConfirmTime, firstWordTime[0]);
			//第一个词是相同，输出
			m_partialNum = 1;
//			m_partialWord[0] = firstWordID[0];
			m_PreConfirmTime = firstWordTime[0];
			m_refAmScore = m_partialResult[0].m_scoreAm[len] / 4;
			m_refLmScore = m_partialResult[0].m_scoreLm[len] / 4;
			//
			count = 0;
			for(i = 1; i < MAX_CONFIRM_BLOCK; i++)
			{
				if(m_partialResult[i].m_nSeqLen >= 4)
				{
					memcpy(m_partialResult+count, m_partialResult+i, sizeof(S_PARTIAL_RESULT));
					m_partialResult[count].m_nSeqLen--;
					count++;
				}
			}
		}
		m_traceCount = count;
		return 1;
	}
	else
	{
		//第一个词不同，把第一次的结果移出
		for(i = 1; i < MAX_CONFIRM_BLOCK; i++)
			memcpy(m_partialResult+i-1, m_partialResult+i, sizeof(S_PARTIAL_RESULT));
		m_traceCount--;
		return 0;
	}
}

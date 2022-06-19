// LM.cpp: implementation of the CLM class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "const.h"
#include "LM.h"
#include "global.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//#define	DEBUG_LM_CACHE
#ifdef	DEBUG_LM_CACHE
int interlmct=0, interfind=0,intertotal=0;
int findCt=0, totalCt=0,totalcycle=0;
#endif

//#define READ_ONLY_BUFFER  //应用缓存，内存少时

CLM::CLM()
{
	m_pUnigram		= NULL;
	m_pBigramIdx	= NULL;
	m_pBigram		= NULL;
	m_pTrigramIdx[0]	= NULL;
	m_pTrigramIdx[1]	= NULL;
	m_pTrigram			= NULL;

#ifdef	USE_LM_VQ
	m_pLmCodebook = NULL;
#endif

#ifdef USE_INDEX_COMPRESS
	m_pTrigramIdxOffset = NULL;
#endif

	for(int i = 0; i < MAX_MODE_NUM; i++)
	{
		m_hFileBigram[i] = NULL;
		m_hFileTrigram[i] = NULL;
		m_hFileMapBigram[i]	= NULL;
		m_hFileMapTrigram[i] = NULL;
	
		m_pUnigramMode[i] = NULL;
		m_pBigramIdxMode[i]	= NULL;
		m_pBigramMode[i] = NULL;
		m_pTrigramIdxMode[i][0] = NULL;
		m_pTrigramIdxMode[i][1] = NULL;
		m_pTrigramMode[i] = NULL;

#ifdef USE_INDEX_COMPRESS
		m_pTrigramIdxOffsetMode[i] = NULL;
#endif
	}

//	m_hHeap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
//	ASSERT(m_hHeap != NULL);

//	memset((LPVOID)m_pKeyHead, 0, sizeof(WORD) * LM_BUFFER_HEAD_SIZE);
//	memset((LPVOID)m_pLMBuffer, 0, sizeof(LM_BUFFER_HEAD) * LM_BUFFER_HEAD_SIZE);
//	m_iKeyHeadNext = 0;
//	m_fBufferFull = FALSE;
	//
#ifdef USE_LM_PREDICT
	memset(pLmCacheHead, 0, MODE_VALUE_LM_CACHE*2);
	memset(lmCacheFull, 0, MODE_VALUE_LM_CACHE);
	//
	memset(pLmCacheHead2, 0, MODE_VALUE_LM_CACHE_SECOND*2);
	memset(lmCacheFull2, 0, MODE_VALUE_LM_CACHE_SECOND);
#endif
}

CLM::~CLM()
{
	for(int i = 0; i < MAX_MODE_NUM; i++)
		UnLoadData(i);
#ifdef	USE_LM_VQ
	if (m_pLmCodebook)
		delete m_pLmCodebook;
#endif
}

void CLM::Init()
{
}

short CLM::LoadData(short loadMode)
{
	TCHAR lpszFileName[MAX_PATH];
	DWORD dwSize;

	switch(loadMode)
	{
		case DICTATION_MODE:
			m_pUnigramMode[loadMode] = ReadFileNoBuffer("UnigramData");
			ASSERT(m_pUnigramMode[loadMode] != NULL);

if(m_pUnigramMode[loadMode] == NULL)
return 10;


			m_pBigramIdxMode[loadMode] = ReadFileNoBuffer("BigramIndex");
			ASSERT(m_pBigramIdxMode[loadMode] != NULL);

 if(m_pBigramIdxMode[loadMode] == NULL)
return 11;

#ifdef READ_ONLY_BUFFER
			m_pBigramMode[loadMode] = ReadFileOnlyBuffer("BigramData", m_hFileBigram[loadMode], m_hFileMapBigram[loadMode]);
#else
			m_pBigramMode[loadMode] = ReadFileNoBuffer("BigramData");
#endif
			ASSERT(m_pBigramMode[loadMode] != NULL);

if(m_pBigramMode[loadMode] == NULL)
return 12;

			m_pTrigramIdxMode[loadMode][0] = ReadFileNoBuffer("TrigramIndex1");
			ASSERT(m_pTrigramIdxMode[loadMode][0] != NULL);

			m_pTrigramIdxMode[loadMode][1] = ReadFileNoBuffer("TrigramIndex2");
			ASSERT(m_pTrigramIdxMode[loadMode][1] != NULL);
if(m_pTrigramIdxMode[loadMode][0] == NULL || m_pTrigramIdxMode[loadMode][1] == NULL)
return 13;


#ifdef USE_INDEX_COMPRESS
			dwSize = GetPrivateProfileString("LM", "TrigramIndex2Offset", NULL, lpszFileName, MAX_PATH, "SearchEngine.ini");
			ASSERT(dwSize != 0);
			FILE *fpp = fopen(lpszFileName, "rb");
			if(fpp == NULL)
				return 1;
			fseek(fpp, 0, SEEK_END);
			m_trigramIdxOffsetNum = ftell(fpp) / sizeof(unsigned int);
			m_pTrigramIdxOffsetMode[loadMode] = new unsigned int[m_trigramIdxOffsetNum];
			fseek(fpp, 0, SEEK_SET);
			fread(m_pTrigramIdxOffsetMode[loadMode], sizeof(unsigned int), m_trigramIdxOffsetNum, fpp);
			fclose(fpp);
#endif


#ifdef READ_ONLY_BUFFER
			m_pTrigramMode[loadMode] = ReadFileOnlyBuffer("TrigramData", m_hFileTrigram[loadMode], m_hFileMapTrigram[loadMode]);
#else
			m_pTrigramMode[loadMode] = ReadFileNoBuffer("Trigramdata");
#endif
			ASSERT(m_pTrigramMode[loadMode] != NULL);
if(m_pTrigramMode[loadMode] == NULL)
return 14;

//			if(m_pUnigramMode[loadMode] == NULL || m_pBigramIdxMode[loadMode] == NULL ||
///				m_pBigramMode[loadMode] == NULL ||m_pTrigramIdxMode[loadMode] == NULL || m_pTrigramMode[loadMode] == NULL)
//				return 1;
			break;
		case COMMAND_MODE:
			break;
		case ADAPT_MODE:
			m_pUnigramMode[loadMode] = ReadFileNoBuffer("UnigramDataAdapt");
			ASSERT(m_pUnigramMode[loadMode] != NULL);

			m_pBigramIdxMode[loadMode] = ReadFileNoBuffer("BigramIndexAdapt");
			ASSERT(m_pBigramIdxMode[loadMode] != NULL);

			//m_pBigram = ReadFileOnlyBuffer("BigramData", m_hFileBigram[loadMode], m_hFileMapBigram[loadMode]);
			m_pBigramMode[loadMode] = ReadFileNoBuffer("BigramDataAdapt");
			ASSERT(m_pBigramMode[loadMode] != NULL);

			m_pTrigramIdxMode[loadMode][0] = ReadFileNoBuffer("TrigramIndexAdapt1");
			m_pTrigramIdxMode[loadMode][1] = ReadFileNoBuffer("TrigramIndexAdapt2");
			ASSERT(m_pTrigramIdxMode[loadMode][0] != NULL);
			ASSERT(m_pTrigramIdxMode[loadMode][1] != NULL);

			//m_pTrigramMode[loadMode] = ReadFileOnlyBuffer("TrigramData", m_hFileTrigram[loadMode], m_hFileMapTrigram[loadMode]);
			m_pTrigramMode[loadMode] = ReadFileNoBuffer("TrigramdataAdapt");
			ASSERT(m_pTrigramMode[loadMode] != NULL);
			if(m_pUnigramMode[loadMode] == NULL || m_pBigramIdxMode[loadMode] == NULL ||
				m_pBigramMode[loadMode] == NULL ||m_pTrigramIdxMode[loadMode][0] == NULL ||m_pTrigramIdxMode[loadMode][0]
				|| m_pTrigramMode[loadMode] == NULL)
				return 1;
			break;
		default:
			break;
	}
	//装载语言模型量化码本,on 8, May.
#ifdef USE_LM_VQ
	//获取文件路径
	dwSize = GetPrivateProfileString("LM", "LmCodebook", NULL, lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);
	FILE *fp = fopen(lpszFileName, "rb");
	if(fp == NULL)
		return 1;
	fseek(fp, 0, SEEK_END);
	int len = ftell(fp) / sizeof(long);
	m_pLmCodebook = new long[len];
	fseek(fp, 0, SEEK_SET);
	fread(m_pLmCodebook, sizeof(long), len, fp);
	fclose(fp);
#endif

	return 0;
}

void CLM::UnLoadData(short loadMode)
{
	switch(loadMode)
	{
		case DICTATION_MODE:
			if (m_pUnigramMode[loadMode])
				VirtualFree(m_pUnigramMode[loadMode], 0, MEM_RELEASE);

			if (m_pBigramIdxMode[loadMode])
				VirtualFree(m_pBigramIdxMode[loadMode], 0, MEM_RELEASE);

			if (m_pTrigramIdxMode[loadMode][0])
				VirtualFree(m_pTrigramIdxMode[loadMode][0], 0, MEM_RELEASE);

			if (m_pTrigramIdxMode[loadMode][1])
				VirtualFree(m_pTrigramIdxMode[loadMode][1], 0, MEM_RELEASE);

			if (m_pBigramMode[loadMode])
				VirtualFree(m_pBigramMode[loadMode], 0, MEM_RELEASE);

			if (m_pTrigramMode[loadMode])
				VirtualFree(m_pTrigramMode[loadMode], 0, MEM_RELEASE);

			CloseFileHandle(loadMode);
#ifdef USE_INDEX_COMPRESS
			delete 	m_pTrigramIdxOffsetMode[loadMode];	
#endif
			break;
		case COMMAND_MODE:
			break;
		default:
			break;
	}
}

// 此函数用来读取Unigram, Bigram Index和Trigram Index，使用 FILE_FLAG_NO_BUFFERING
// 目的： 加快数据使用的速度
LPBYTE CLM::ReadFileNoBuffer(LPCTSTR lpszKey)
{
	//计算扇区的大小
	DWORD dwSecPerCluster, dwBytePerSector, dwNumFreeCluster, dwNumTotalCluster;
	if (!GetDiskFreeSpace(NULL, &dwSecPerCluster, &dwBytePerSector, &dwNumFreeCluster, 
		&dwNumTotalCluster)){
		TRACE1("There is error when GetDiskFreeSpace(), Error num = %d\n", GetLastError());
		return NULL;
	}

	//获取文件路径
	TCHAR lpszFileName[MAX_PATH];
	DWORD dwSize = GetPrivateProfileString("LM", lpszKey, NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);

	//打开文件并获取文件大小
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
							  OPEN_EXISTING, 
							  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, 
							  NULL);	
	if (hFile == INVALID_HANDLE_VALUE){
		return NULL;
	}

	dwSize = GetFileSize(hFile, NULL);
	ASSERT(dwSize != INVALID_FILE_SIZE);

	//分配内存
	LPBYTE pbDes = (LPBYTE)VirtualAlloc(NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (pbDes == NULL){
		TRACE("There is not enough memory!\n");
		CloseHandle(hFile);
		return NULL;
	}

	//计算文件所占扇区个数
	int	nBuffer = dwSize / dwBytePerSector;
	if (dwSize % dwBytePerSector)
		nBuffer++;

	//读取到缓冲区 
	DWORD dwNumByteRead;
	if (!ReadFile(hFile, pbDes, dwBytePerSector * nBuffer, &dwNumByteRead, NULL)){
		TRACE1("Error when read Bigram Index file, Error Num = %d\n", GetLastError());
		CloseHandle(hFile);
		VirtualFree(pbDes, 0, MEM_RELEASE);
		return NULL;
	}

	//关闭文件
	CloseHandle(hFile);

	return pbDes;
}

//使用FileMapping读取Bigram 数据和Trigram数据，使用系统的内存。
//目的： 减少内存的使用量。
LPBYTE CLM::ReadFileOnlyBuffer(LPCTSTR lpszKey, HANDLE &hFile, HANDLE &hFileMap)
{
	ASSERT(lpszKey != NULL);
	ASSERT(hFile == NULL);
	ASSERT(hFileMap == NULL);

	//获取文件路径
	TCHAR lpszFileName[MAX_PATH];
	DWORD dwSize = GetPrivateProfileString("LM", lpszKey, NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);

	//使用"random access"方式打开文件
	hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
							  OPEN_EXISTING, 
							  FILE_FLAG_RANDOM_ACCESS, //FILE_ATTRIBUTE_NORMAL,// | FILE_FLAG_SEQUENTIAL_SCAN,
							  NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		TRACE2("Error when CreateFile, Error Number = %d, lpszKey = %s\n",
				GetLastError(), lpszFileName);
		hFile = NULL;
		return NULL;
	}
	
	// 创建文件映射对象
	hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hFileMap == NULL){
		TRACE2("Error when CreateFileMapping, Error Number = %d, lpszKey = %s\n", 
				GetLastError(), lpszFileName);
		CloseHandle(hFile);
		return NULL;
	}

	// 将文件数据映射到进程的内存空间
	LPBYTE pbFile = (LPBYTE)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
	if (pbFile == NULL){
		TRACE2("Error when MapViewOfFile, Error Number = %d, lpszKey = %s\n",
				GetLastError(), lpszFileName);
		CloseHandle(hFileMap);
		hFileMap = NULL;
		CloseHandle(hFile);
		hFile = NULL;
		return NULL;
	}

	return pbFile;

}

//关闭Bigram数据和Trigram数据的Handle
void CLM::CloseFileHandle(short loadMode)
{
	if (m_pBigramMode[loadMode] != NULL){
		UnmapViewOfFile(m_pBigramMode[loadMode]);
		CloseHandle(m_hFileMapBigram[loadMode]);
		CloseHandle(m_hFileBigram[loadMode]);
	}

	if (m_pTrigramMode[loadMode] != NULL){
		UnmapViewOfFile(m_pTrigramMode[loadMode]);
		CloseHandle(m_hFileMapTrigram[loadMode]);
		CloseHandle(m_hFileTrigram[loadMode]);
	}
}

long CLM::GetLMScore(WORD wWord1, WORD wWord2, WORD wWord3, long lProbBackoff)
{
	long lLMScore;
	//直接计算三元概率
	if(g_pModeManager->GetActiveMode() == COMMAND_MODE)
		lLMScore = 0;
	else
	{
		lLMScore = CheckTrigram(wWord1, wWord2, wWord3);

		if (lLMScore == NEG_HUGE){		//返回到二元概率
			lLMScore = CheckBigram(wWord2, wWord3);

			if (lLMScore == NEG_HUGE){	//返回到一元概率
#ifdef	USE_LM_VQ
				lLMScore = m_pLmCodebook[*(CODEWORD_SIZE*)(m_pUnigram + wWord3 * 2L)] + 
					   m_pLmCodebook[*(CODEWORD_SIZE*)(m_pUnigram + wWord2 * 2L + 1)];

#else
				lLMScore = (*(long*)(m_pUnigram + wWord3 * 8L)) + 
					   (*(long*)(m_pUnigram + wWord2 * 8L + 4));
#endif
			}
			lLMScore += lProbBackoff;
		}
	}
	return lLMScore;
}

#ifdef TRIGRAM_SECOND_INDEX
inline long CLM::CheckTrigram(WORD w1, WORD w2, WORD w3)
{
	unsigned char   *pTmp, bFound = 0;
	long  offSet, offset2;
	int num, num2, offsetidx1, offsetidx2;
	long word_ptr,Ptri;

	// Just check the most favorate one
	num = *(WORD *)(m_pTrigramIdx[0] + w1 * 6);
	offSet = *(long*)(m_pTrigramIdx[0] + w1 * 6 + 2);
#ifdef USE_INDEX_COMPRESS
	pTmp = m_pTrigramIdx[1] + offSet * 3;
#else
	pTmp = m_pTrigramIdx[1] + offSet * 8;
#endif

	word_ptr = 0;

	//changed by deng yong gang, binary search:
	//modified by zhang hong, because of the change of data structure.
	int low = 0;
	int high = num-1;
	int mid;
	//BOOL bFound = FALSE;
	//find the first idx2
	Ptri = NEG_HUGE;
	while( low <= high ) 
	{
		mid = (low + high) / 2;
#ifdef USE_INDEX_COMPRESS
		WORD nCurIdx2 = *(WORD *)(pTmp + mid * 3);
#else
		WORD nCurIdx2 = *(WORD *)(pTmp + mid * 8);
#endif
		if (nCurIdx2 == w2)	
		{
			bFound = 1;
#ifdef USE_INDEX_COMPRESS
			offsetidx1 = FindTrigramIdxOffset(offSet + mid, 0, m_trigramIdxOffsetNum - 1);
			ASSERT(offsetidx1 >= 0);
			offsetidx2 = FindTrigramIdxOffset(offSet + mid + 1, offsetidx1, m_trigramIdxOffsetNum - 1);
			ASSERT(offsetidx2 >= 0);
			
			offsetidx1 = ((offsetidx1 - 1) << 8) + (*(unsigned char*)(pTmp + mid * 3 + 2));
			offsetidx2 = ((offsetidx2 - 1) << 8) + (*(unsigned char*)(pTmp + (mid+1) * 3 + 2));
			num2 = offsetidx2 - offsetidx1;
			ASSERT(num2 > 0);
			offset2 = offsetidx1;
#else
			num2 = *(WORD *)(pTmp + mid * 8 + 2);
			offset2 = *(long *)(pTmp + mid * 8 + 4);
#endif
			break;
		}
		else if (nCurIdx2 > w2) 
			high = mid-1; //continue in low interval
		else 
			low = mid + 1; //continue in high interval
	}// end of while
	if(bFound)
	{
		//
#ifdef	USE_LM_VQ
		pTmp = m_pTrigram + offset2 * 3;
#else
		pTmp = m_pTrigram + offset2 * 6;
#endif

		low = 0;
		high = num2-1;
		while( low <= high ) 
		{
			mid = (low + high) / 2;
#ifdef	USE_LM_VQ
			WORD nCurIdx2 = *(WORD *)(pTmp + mid * 3);
#else
			WORD nCurIdx2 = *(WORD *)(pTmp + mid * 6);
#endif

			if (nCurIdx2 == w3)	
			{
#ifdef	USE_LM_VQ
				Ptri = m_pLmCodebook[*(CODEWORD_SIZE*)(pTmp + mid * 3 + 2)];
#else
				Ptri = *(long *)(pTmp + mid * 6 + 2);
#endif
				break;
			}
			else if (nCurIdx2 > w3) 
				high = mid-1; //continue in low interval
			else 
				low = mid + 1; //continue in high interval
		}// end of while
	}
	return Ptri;
}

#else
inline long CLM::CheckTrigram(WORD w1, WORD w2, WORD w3)
{
	unsigned char   *pTmp;
	long  offSet;
	int num;
	long word_ptr,Ptri;

	// Just check the most favorate one
	offSet = *(long*)(m_pTrigramIdx[0] + w1 * 8);
	num = *(DWORD *)(m_pTrigramIdx[0] + w1 * 8 + 4);
	pTmp = m_pTrigram + offSet * 8;
	word_ptr = 0;

	//changed by deng yong gang, binary search:
	//modified by zhang hong, because of the change of data structure.
	int low = 0;
	int high = num-1;
	int mid;
	//BOOL bFound = FALSE;
	//find the first idx2
	Ptri = NEG_HUGE;
	while( low <= high ) 
	{
		mid = (low + high) / 2;
		WORD nCurIdx2 = *(WORD *)(pTmp + mid * 8);
		WORD nCurIdx3 = *(WORD *)(pTmp + mid * 8 + 2);//add by zh

		if (nCurIdx2 == w2 && nCurIdx3 == w3)	
		{//edit by zh
			// found here!
			Ptri=*(long *)(pTmp+mid*8+4);
			if(Ptri == -19980049)
				Ptri = NEG_HUGE;
			break;
		}
		else if (nCurIdx2 > w2) 
		{//continue in low interval
			high = mid-1; //continue in low interval
		}
		else if (nCurIdx2 == w2 && nCurIdx3 > w3) 
		{//continue in low interval---- add by zh
			high = mid-1; //continue in low interval---add by zh
		}
		else 
		{
			low = mid + 1; //continue in high interval
		}
	}// end of while

	return Ptri;
}
#endif


inline long CLM::CheckBigram(WORD w1, WORD w2)
{
	long offSet, probLmMax;
	BYTE *pTmp;
	int num, low, high, mid;
	WORD nCurIdx2;

	// Just check the most favorate one
	offSet = *(long*)(m_pBigramIdx + w1 * 6L);
	num = *(WORD *)(m_pBigramIdx + w1 * 6L+ 4);

#ifdef	USE_LM_VQ
	pTmp = m_pBigram + offSet * 4L;
#else
	pTmp = m_pBigram + offSet * 10L;
#endif

	probLmMax = NEG_HUGE;
	if(num > 0)
	{
		nCurIdx2 = *(WORD *)(pTmp);
		if(w2 < nCurIdx2)
		{
		//	if(probLmMax == -19980049)
		//		probLmMax = NEG_HUGE;
			return probLmMax;
		}
#ifdef	USE_LM_VQ
		nCurIdx2 = *(WORD *)(pTmp + (num-1)*4);
#else
		nCurIdx2 = *(WORD *)(pTmp + (num-1)*10);
#endif
		if(w2 > nCurIdx2)
		{
		//	if(probLmMax == -19980049)
		//		probLmMax = NEG_HUGE;
			return probLmMax;
		}

		//changed by deng yong gang, binary search:
		//modified by zhang hong, because of the change of data structure.
		low = 0;
		high = num-1;

		//find the first idx2
		while( low <= high ) 
		{
			mid = (low + high) / 2;
#ifdef	USE_LM_VQ
			nCurIdx2 = *(WORD *)(pTmp + mid * 4);
#else
			nCurIdx2 = *(WORD *)(pTmp + mid * 10);
#endif

			if (nCurIdx2 == w2)
			{
#ifdef	USE_LM_VQ
				probLmMax = m_pLmCodebook[*(CODEWORD_SIZE*)(pTmp + mid * 4 + 2)];
#else
				probLmMax = *(long*)(pTmp + mid * 10 + 2);
#endif
				break;
			}
			else if (nCurIdx2 > w2) 
			{
				high = mid - 1; //continue in low interval
			}
			else 
			{
				low = mid + 1; //continue in high interval
			}
		}// end of while
	}
	if(probLmMax == -19980049)
		probLmMax = NEG_HUGE;
	
	return probLmMax;
}

long CLM::CheckTriBackoff(WORD w1, WORD w2)
{
	long offSet, probLmMax;
	BYTE *pTmp;
	int num, low, high, mid;
	WORD nCurIdx2;

	if(g_pModeManager->GetActiveMode() == COMMAND_MODE)
		return 0;	

	// Just check the most favorate one
	offSet = *(long*)(m_pBigramIdx + w1 * 6L);
	num = *(WORD *)(m_pBigramIdx + w1 * 6L+ 4);

#ifdef	USE_LM_VQ
	pTmp = m_pBigram + offSet * 4L;
#else
	pTmp = m_pBigram + offSet * 10L;
#endif

	probLmMax = NEG_HUGE;
	if(num > 0)
	{
		nCurIdx2 = *(WORD *)(pTmp);
		if(w2 < nCurIdx2)
		{
			//if(probLmMax == -19980049)
			//	probLmMax = NEG_HUGE;
			return 0;
		}
#ifdef	USE_LM_VQ
		nCurIdx2 = *(WORD *)(pTmp + (num-1)*4);
#else
		nCurIdx2 = *(WORD *)(pTmp + (num-1)*10);
#endif

		if(w2 > nCurIdx2)
		{
			//if(probLmMax == -19980049)
			//	probLmMax = NEG_HUGE;
			return 0;
		}

		//changed by deng yong gang, binary search:
		//modified by zhang hong, because of the change of data structure.
		low = 0;
		high = num-1;

		//find the first idx2
		while( low <= high ) 
		{
			mid = (low + high) / 2;
#ifdef	USE_LM_VQ
			nCurIdx2 = *(WORD*)(pTmp + mid * 4);
#else
			nCurIdx2 = *(WORD*)(pTmp + mid * 10);
#endif

			if (nCurIdx2 == w2)
			{
#ifdef	USE_LM_VQ
				probLmMax = m_pLmCodebook[*(CODEWORD_SIZE*)(pTmp + mid * 4 + 3)];
#else
				probLmMax = *(long*)(pTmp + mid * 10 + 6);
#endif
				break;
			}
			else if (nCurIdx2 > w2) 
			{
				high = mid - 1; //continue in low interval
			}
			else 
			{
				low = mid + 1; //continue in high interval
			}
		}// end of while
	}
	if(probLmMax == -19980049)
		probLmMax = NEG_HUGE;
	if(probLmMax == NEG_HUGE)
		probLmMax = 0;
	return probLmMax;
}

/*/
long CLM::GetLMPredictScore(WORD w1, WORD w2, int node, 
							WORD xWord, WORD yWord, WORD MaxUnigramID, long triBackoffWeight)
{
	long probLmMax;
//	if (GetDisFromBuffer(w1, w2, nGrammarNode, probLmMax))
//		return probLmMax;
	int i, cacheNum, curHead;

	curHead = pLmCacheHead[node];
	if(lmCacheFull[node])
		cacheNum = MAX_CACHE_LIST;
	else
		cacheNum = curHead;

	for(i = 0; i < cacheNum; i++)
		if(lmCache[node][i].w1 == w1 && lmCache[node][i].w2 == w2)
			break;

	//totalCt++;
	if(i != cacheNum)
	{
		probLmMax = lmCache[node][i].prob;
		//findCt++;
	}
	else
	{

		WORD maxWord = 0;
		probLmMax = GetDisTrigram(w1, w2, xWord, yWord);

		if (probLmMax == NEG_HUGE){	//
			probLmMax  = GetDisBigram(w2, xWord, yWord);

			if (probLmMax == NEG_HUGE){	//
				long BiBackoffWeight = *(long*)(m_pUnigram + w2 * 8L + 4);
				long lMaxUnigramProb = (*(long*)(m_pUnigram + MaxUnigramID * 8L));
				probLmMax = lMaxUnigramProb + BiBackoffWeight + triBackoffWeight;
				//maxWord = MaxUnigramID;

			}
			else {
				probLmMax += triBackoffWeight;
			}
		}
		//not find in the cache
		lmCache[node][curHead].w1 = w1;
		lmCache[node][curHead].w2 = w2;
		lmCache[node][curHead].prob = probLmMax;
		pLmCacheHead[node]++;
		if(pLmCacheHead[node] == MAX_CACHE_LIST)
		{
			lmCacheFull[node] = 1;
			pLmCacheHead[node] = 0;
		}
	}
	//InsertBuffer(w1, w2, nGrammarNode, probLmMax);

	return probLmMax;
}
*/
/*/cache 1
long CLM::GetLMPredictScore(WORD w1, WORD w2, int node, 
							WORD xWord, WORD yWord, WORD MaxUnigramID, long triBackoffWeight)
{
	long probLmMax;
//	if (GetDisFromBuffer(w1, w2, nGrammarNode, probLmMax))
//		return probLmMax;
	int i, cacheNum, curHead;

	curHead = pLmCacheHead[node];
	if(lmCacheFull[node])
		cacheNum = MAX_CACHE_LIST;
	else
		cacheNum = curHead;

	for(i = 0; i < cacheNum; i++)
	{
		totalCt++;
		if(lmCache[node][i].w1 == w1 && lmCache[node][i].w2 == w2)
			break;
	}
//	totalCt++;
	if(i != cacheNum)
	{
		probLmMax = lmCache[node][i].prob;
		findCt++;
	}
	else
	{
		WORD maxWord = 0;
		probLmMax = GetDisTrigram(w1, w2, xWord, yWord);

		if (probLmMax == NEG_HUGE){	//
			probLmMax  = GetDisBigram(w2, xWord, yWord);

			if (probLmMax == NEG_HUGE){	//
				long BiBackoffWeight = *(long*)(m_pUnigram + w2 * 8L + 4);
				long lMaxUnigramProb = (*(long*)(m_pUnigram + MaxUnigramID * 8L));
				probLmMax = lMaxUnigramProb + BiBackoffWeight + triBackoffWeight;
				//maxWord = MaxUnigramID;

			}
			else {
				probLmMax += triBackoffWeight;
			}
		}
		//not find in the cache
		lmCache[node][curHead].w1 = w1;
		lmCache[node][curHead].w2 = w2;
		lmCache[node][curHead].prob = probLmMax;
		pLmCacheHead[node]++;
		if(pLmCacheHead[node] == MAX_CACHE_LIST)
		{
			lmCacheFull[node] = 1;
			pLmCacheHead[node] = 0;
		}
	}
	//InsertBuffer(w1, w2, nGrammarNode, probLmMax);

	return probLmMax;
}
*/

/*
long CLM::GetLMPredictScore(WORD w1, WORD w2, int node, 
							WORD xWord, WORD yWord, WORD MaxUnigramID, long triBackoffWeight)
{
	long probLmMax;
	int i, cacheNum, curHead, key;
	unsigned int wordidx;;

	if(g_pModeManager->GetActiveMode() == COMMAND_MODE)
		return 0;

#ifdef USE_LM_PREDICT
	totalcycle++;
	key = (node + w1 * w2) % MODE_VALUE_LM_CACHE;
	curHead = pLmCacheHead[key];
	if(lmCacheFull[key])
		cacheNum = MAX_CACHE_LIST;
	else
		cacheNum = curHead;

	wordidx = (unsigned int)(w1);
	wordidx = wordidx << 16 + w2;
	for(i = 0; i < cacheNum; i++)
	{
		totalCt++;
		//if(lmCache[key][i].word == wordidx && lmCache[key][i].node == node)
		if(lmCache[key][i].node == node)
			if(lmCache[key][i].word == wordidx)
				break;
		
	}

	//totalCt++;
	if(i != cacheNum)
	{
		probLmMax = lmCache[key][i].prob;
		findCt++;
	}
	else
	{
		probLmMax = GetDisTrigram(w1, w2, xWord, yWord);

		if (probLmMax == NEG_HUGE){	//
			probLmMax  = GetDisBigram(w2, xWord, yWord);

			if (probLmMax == NEG_HUGE){	//
				long BiBackoffWeight = *(long*)(m_pUnigram + w2 * 8L + 4);
				long lMaxUnigramProb = (*(long*)(m_pUnigram + MaxUnigramID * 8L));
				probLmMax = lMaxUnigramProb + BiBackoffWeight + triBackoffWeight;
			}
			else {
				probLmMax += triBackoffWeight;
			}
		}
		//not find in the cache
		lmCache[key][curHead].word = wordidx;
		lmCache[key][curHead].node = node;
		lmCache[key][curHead].prob = probLmMax;
		pLmCacheHead[key]++;
		if(pLmCacheHead[key] == MAX_CACHE_LIST)
		{
			lmCacheFull[key] = 1;
			pLmCacheHead[key] = 0;
		}
		
	//	pLmCacheHead[key] = pLmCacheHead[key] % MAX_CACHE_LIST;
	}
	//InsertBuffer(w1, w2, nGrammarNode, probLmMax);
#else
		probLmMax = GetDisTrigram(w1, w2, xWord, yWord);

		if (probLmMax == NEG_HUGE){	//
			probLmMax  = GetDisBigram(w2, xWord, yWord);

			if (probLmMax == NEG_HUGE){	//
				long BiBackoffWeight = *(long*)(m_pUnigram + w2 * 8L + 4);
				long lMaxUnigramProb = (*(long*)(m_pUnigram + MaxUnigramID * 8L));
				probLmMax = lMaxUnigramProb + BiBackoffWeight + triBackoffWeight;
			}
			else {
				probLmMax += triBackoffWeight;
			}
		}
#endif
	return probLmMax;

}
*/
long CLM::GetLMPredictScore(WORD w1, WORD w2, int node, 
							WORD xWord, WORD yWord, WORD MaxUnigramID, long triBackoffWeight)
{
	long probLmMax;
	int i, cacheNum, curHead, key;
	unsigned int wordidx;;

	if(g_pModeManager->GetActiveMode() == COMMAND_MODE)
		return 0;

#ifdef USE_LM_PREDICT

#ifdef	DEBUG_LM_CACHE
	totalcycle++;
#endif
	key = (node + w1 * w2) % MODE_VALUE_LM_CACHE;
	curHead = pLmCacheHead[key];
	if(lmCacheFull[key])
		cacheNum = MAX_CACHE_LIST;
	else
		cacheNum = curHead;

	wordidx = (unsigned int)(w1);
	wordidx = wordidx << 16 + w2;
	for(i = 0; i < cacheNum; i++)
	{
#ifdef	DEBUG_LM_CACHE
		totalCt++;
#endif
		//if(lmCache[key][i].word == wordidx && lmCache[key][i].node == node)
		if(lmCache[key][i].node == node)
			if(lmCache[key][i].word == wordidx)
				break;
		
	}

	if(i != cacheNum)
	{
		probLmMax = lmCache[key][i].prob;
#ifdef	DEBUG_LM_CACHE
		findCt++;
#endif
	}
	else
	{
		probLmMax = GetDisTrigram(w1, w2, xWord, yWord);

		if (probLmMax == NEG_HUGE){	//
			probLmMax  = GetDisBigram(w2, xWord, yWord);

			if (probLmMax == NEG_HUGE){	//
#ifdef	USE_LM_VQ
				long BiBackoffWeight = m_pLmCodebook[*(CODEWORD_SIZE*)(m_pUnigram + w2 * 2L + 1)];
				long lMaxUnigramProb = m_pLmCodebook[*(CODEWORD_SIZE*)(m_pUnigram + MaxUnigramID * 2L)];
#else
				long BiBackoffWeight = *(long*)(m_pUnigram + w2 * 8L + 4);
				long lMaxUnigramProb = (*(long*)(m_pUnigram + MaxUnigramID * 8L));
#endif
				probLmMax = lMaxUnigramProb + BiBackoffWeight + triBackoffWeight;

			}
			else {
				probLmMax += triBackoffWeight;
			}
		}
		//not find in the cache
		lmCache[key][curHead].word = wordidx;
		lmCache[key][curHead].node = node;
		lmCache[key][curHead].prob = probLmMax;
		pLmCacheHead[key]++;
		if(pLmCacheHead[key] == MAX_CACHE_LIST)
		{
			lmCacheFull[key] = 1;
			pLmCacheHead[key] = 0;
		}
		
	//	pLmCacheHead[key] = pLmCacheHead[key] % MAX_CACHE_LIST;
	}
	//InsertBuffer(w1, w2, nGrammarNode, probLmMax);
#else
		probLmMax = GetDisTrigram(w1, w2, xWord, yWord);

		if (probLmMax == NEG_HUGE){	//
			probLmMax  = GetDisBigram(w2, xWord, yWord);

			if (probLmMax == NEG_HUGE){	//
#ifdef	USE_LM_VQ
				long BiBackoffWeight = m_pLmCodebook[*(CODEWORD_SIZE*)(m_pUnigram + w2 * 2L + 1)];
				long lMaxUnigramProb = m_pLmCodebook[*(CODEWORD_SIZE*)(m_pUnigram + MaxUnigramID * 2L)];
#else
				long BiBackoffWeight = *(long*)(m_pUnigram + w2 * 8L + 4);
				long lMaxUnigramProb = (*(long*)(m_pUnigram + MaxUnigramID * 8L));
#endif
				probLmMax = lMaxUnigramProb + BiBackoffWeight + triBackoffWeight;
			}
			else {
				probLmMax += triBackoffWeight;
			}
		}
#endif
	return probLmMax;

}

#ifdef TRIGRAM_SECOND_INDEX
long CLM::GetDisTrigram(WORD w1, WORD w2, WORD xWord, WORD yWord)
{
	BYTE   *pTmp;
	long  offSet, offset2;
	int num, num2, offsetidx1, offsetidx2;
	long word_ptr,Ptri, probLmMax;
	
	if(g_pModeManager->GetActiveMode() == COMMAND_MODE)
		return 0;

	// Just check the most favorate one
	num = *(WORD *)(m_pTrigramIdx[0] + w1 * 6);
	offSet = *(long*)(m_pTrigramIdx[0] + w1 * 6 + 2);
#ifdef USE_INDEX_COMPRESS
	pTmp = m_pTrigramIdx[1] + offSet * 3;
#else
	pTmp = m_pTrigramIdx[1] + offSet * 8;
#endif
	word_ptr = 0;

	//changed by deng yong gang, binary search:
	//modified by zhang hong, because of the change of data structure.
	int low = 0;
	int high = num-1;
	int mid;
	BOOL bFound = FALSE;
	DWORD nCurIdx2, nCurIdx3;

	probLmMax = NEG_HUGE;
	//find the first idx2
	while( low <= high ) 
	{
		mid = (low + high) / 2;
#ifdef USE_INDEX_COMPRESS
		nCurIdx2 = *(WORD *)(pTmp + mid * 3);
#else
		nCurIdx2 = *(WORD *)(pTmp + mid * 8);
#endif

		if (nCurIdx2 == w2)
		{//edit by zh
			// found here!
//			Ptri=*(long *)(pTmp+mid*8+4);
#ifdef USE_INDEX_COMPRESS
			offsetidx1 = FindTrigramIdxOffset(offSet + mid, 0, m_trigramIdxOffsetNum - 1);
			ASSERT(offsetidx1 >= 0);
			offsetidx2 = FindTrigramIdxOffset(offSet + mid + 1, offsetidx1, m_trigramIdxOffsetNum - 1);
			ASSERT(offsetidx2 >= 0);

			offsetidx1 = ((offsetidx1 - 1) << 8) + (*(unsigned char*)(pTmp + mid * 3 + 2));
			offsetidx2 = ((offsetidx2 - 1) << 8) + (*(unsigned char*)(pTmp + (mid+1) * 3 + 2));
			num2 = offsetidx2 - offsetidx1;
			ASSERT(num2 > 0);
			offset2 = offsetidx1;
#else
			offset2 = *(long *)(pTmp + mid * 8 + 4);
			num2 = *(WORD *)(pTmp + mid * 8 + 2);
#endif
			bFound = TRUE;
			break;
		}
		else if (nCurIdx2 > w2) 
		{//continue in low interval
			high = mid-1; //continue in low interval
		}
		else 
		{
			low = mid + 1; //continue in high interval
		}
	}// end of while

	if(bFound)
	{
#ifdef	USE_LM_VQ
		pTmp = m_pTrigram + offset2 * 3;
#else
		pTmp = m_pTrigram + offset2 * 6;
#endif

		low = 0;
		high = num2-1;

		while (low <= high){
			mid = (low + high) / 2;
#ifdef	USE_LM_VQ
			nCurIdx2 = *(WORD*)(pTmp + mid * 3);
#else
			nCurIdx2 = *(WORD*)(pTmp + mid * 6);
#endif

			if ((nCurIdx2 >= xWord) && (nCurIdx2 <= yWord)){
#ifdef	USE_LM_VQ
				probLmMax = m_pLmCodebook[*(CODEWORD_SIZE*)(pTmp + mid * 3 + 2)];
#else
				probLmMax = *(long*)(pTmp + mid * 6 + 2);
#endif
				//向前顺序遍历
				low = mid - 1;
#ifdef	USE_LM_VQ
				nCurIdx2 = *(WORD*)(pTmp + low * 3);
#else				
				nCurIdx2 = *(WORD*)(pTmp + low * 6);
#endif

				while ((low >= 0) && (nCurIdx2 >= xWord)){
#ifdef	USE_LM_VQ
					long prob = m_pLmCodebook[*(CODEWORD_SIZE*)(pTmp + low * 3 + 2)];
#else
					long prob = *(long*)(pTmp + low * 6 + 2);
#endif
					if(probLmMax < prob){
						probLmMax = prob;
					}

					low--;
#ifdef	USE_LM_VQ
					nCurIdx2 = *(WORD*)(pTmp + low * 3);
#else
					nCurIdx2 = *(WORD*)(pTmp + low * 6);
#endif
				}

				//向后顺序遍历
				low = mid + 1;
#ifdef	USE_LM_VQ
				nCurIdx2 = *(WORD*)(pTmp + low * 3);
#else				
				nCurIdx2 = *(WORD*)(pTmp + low * 6);
#endif
				while ((low <= num2 - 1) && (nCurIdx2 <= yWord)){
#ifdef	USE_LM_VQ
					long prob = m_pLmCodebook[*(CODEWORD_SIZE*)(pTmp + low * 3 + 2)];
#else
					long prob = *(long*)(pTmp + low * 6 + 2);
#endif
					if (probLmMax < prob){
						probLmMax = prob;
						//*maxWord = nCurIdx2;
					}

					low++;
#ifdef	USE_LM_VQ
					nCurIdx2 = *(WORD*)(pTmp + low * 3);
#else
					nCurIdx2 = *(WORD*)(pTmp + low * 6);
#endif
				}

				break;

			}
			if (nCurIdx2 > yWord)
				high = mid - 1;
			else
				low = mid + 1;
		}
	}

	if(probLmMax == -19980049)
		probLmMax = NEG_HUGE;
	return probLmMax;
}
#else

long CLM::GetDisTrigram(WORD w1, WORD w2, WORD xWord, WORD yWord)
{
	BYTE   *pTmp;
	long  offSet;
	int num;
	long word_ptr,Ptri, probMax;
	
	if(g_pModeManager->GetActiveMode() == COMMAND_MODE)
		return 0;

	// Just check the most favorate one
	offSet = *(long*)(m_pTrigramIdx[0] + w1 * 8);
	num = *(DWORD *)(m_pTrigramIdx[0] + w1 * 8 + 4);
	pTmp = m_pTrigram + offSet * 8;
	word_ptr = 0;

	//changed by deng yong gang, binary search:
	//modified by zhang hong, because of the change of data structure.
	int low = 0;
	int high = num-1;
	int mid;
	BOOL bFound = FALSE;
	DWORD nCurIdx2, nCurIdx3;

//	PTri = neg_huge;
	//find the first idx2
	while( low <= high ) 
	{
		mid = (low + high) / 2;
		nCurIdx2 = *(WORD *)(pTmp + mid * 8);
		//WORD nCurIdx3 = *(WORD*)(pTmp + mid * 8 + 2);//add by zh

		if (nCurIdx2 == w2)
		{//edit by zh
			// found here!
//			Ptri=*(long *)(pTmp+mid*8+4);
			bFound = TRUE;
			break;
		}
		else if (nCurIdx2 > w2) 
		{//continue in low interval
			high = mid-1; //continue in low interval
		}
		else 
		{
			low = mid + 1; //continue in high interval
		}
	}// end of while

	if(!bFound)
		return NEG_HUGE;

	probMax = NEG_HUGE;
	int index = mid;
	//while(1)
	while(index >= 0)
	{
		nCurIdx2 = *(WORD *)(pTmp + index * 8);
		nCurIdx3 = *(WORD *)(pTmp + index * 8 + 2);
		if(nCurIdx2 != w2)
			break;
		if(nCurIdx3 < xWord)
			break;
		if(nCurIdx3 <= yWord)
		{	
			Ptri = *(long *)(pTmp + index * 8 + 4);
			if(Ptri > probMax){
				probMax = Ptri;
				//*maxWord = nCurIdx3;
			}
		}
		index--;
	}
	index = mid + 1;
	while(index <= num - 1)//1)
	{
		nCurIdx2 = *(WORD *)(pTmp + index * 8);
		nCurIdx3 = *(WORD*)(pTmp + index * 8 + 2);
		if(nCurIdx2 != w2)
			break;
		if(nCurIdx3 > yWord)
			break;
		if(nCurIdx3 >= xWord)
		{
			Ptri = *(long *)(pTmp + index * 8 + 4);
			if(Ptri > probMax){
				probMax = Ptri;
				//*maxWord = nCurIdx3;
			}
		}
		index++;
	}
	if(probMax == -19980049)
		probMax = NEG_HUGE;
	return probMax;
}


#endif

/////////////////////////////////////////////////////
long CLM::GetDisBigram(WORD w1, WORD xWord, WORD yWord)
{
	long offSet, probLmMax, probTmp;
	BYTE   *pTmp;
	int num, low, high, mid, index, lowLevel, highLevel;
	DWORD nCurIdx2, rword, wdtmp;

	// Just check the most favorate one
	offSet = *(long*)(m_pBigramIdx + w1 * 6L);
	num = *(WORD *)(m_pBigramIdx + w1 * 6L+ 4);

#ifdef	USE_LM_VQ
	pTmp = m_pBigram + offSet * 4L;
#else
	pTmp = m_pBigram + offSet * 10L;
#endif

	probLmMax = NEG_HUGE;
	if(num > 0)
	{
		nCurIdx2 = *(WORD*)(pTmp);
		if(nCurIdx2 > yWord)
		{
			//if(probLmMax == -19980049)
			//	probLmMax = NEG_HUGE;
			//return probLmMax;
			return NEG_HUGE;
		}

#ifdef	USE_LM_VQ
		nCurIdx2 = *(WORD*)(pTmp + (num-1)*4);
#else
		nCurIdx2 = *(WORD*)(pTmp + (num-1)*10);
#endif
		if(nCurIdx2 < xWord)
		{
			//if(probLmMax == -19980049)
			//	probLmMax = NEG_HUGE;
			//return probLmMax;
			return NEG_HUGE;
		}

		//changed by deng yong gang, binary search:
		//modified by zhang hong, because of the change of data structure.
		low = 0;
		high = num-1;
		while (low <= high){
			mid = (low + high) / 2;
#ifdef	USE_LM_VQ
			nCurIdx2 = *(WORD*)(pTmp + mid * 4);
#else
			nCurIdx2 = *(WORD*)(pTmp + mid * 10);
#endif

			if ((nCurIdx2 >= xWord) && (nCurIdx2 <= yWord)){
#ifdef	USE_LM_VQ
				probLmMax = m_pLmCodebook[*(CODEWORD_SIZE*)(pTmp + mid * 4 + 2)];
#else
				probLmMax = *(long*)(pTmp + mid * 10 + 2);
				//*maxWord = nCurIdx2;
#endif

				//向前顺序遍历
				low = mid - 1;
#ifdef	USE_LM_VQ
				nCurIdx2 = *(WORD*)(pTmp + low * 4);
#else
				nCurIdx2 = *(WORD*)(pTmp + low * 10);
#endif
				while ((low >= 0) && (nCurIdx2 >= xWord)){
#ifdef	USE_LM_VQ
					long prob = m_pLmCodebook[*(CODEWORD_SIZE*)(pTmp + low * 4 + 2)];
#else
					long prob = *(long*)(pTmp + low * 10 + 2);
#endif
					if(probLmMax < prob){
						probLmMax = prob;
						//*maxWord = nCurIdx2;
					}

					low--;
#ifdef	USE_LM_VQ
					nCurIdx2 = *(WORD*)(pTmp + low * 4);
#else
					nCurIdx2 = *(WORD*)(pTmp + low * 10);
#endif
				}

				//向后顺序遍历
				low = mid + 1;
#ifdef	USE_LM_VQ
				nCurIdx2 = *(WORD*)(pTmp + low * 4);
#else
				nCurIdx2 = *(WORD*)(pTmp + low * 10);
#endif
				while ((low <= num - 1) && (nCurIdx2 <= yWord)){
#ifdef	USE_LM_VQ
					long prob = m_pLmCodebook[*(CODEWORD_SIZE*)(pTmp + low * 4 + 2)];
#else
					long prob = *(long*)(pTmp + low * 10 + 2);
#endif
					if (probLmMax < prob){
						probLmMax = prob;
						//*maxWord = nCurIdx2;
					}

					low++;
#ifdef	USE_LM_VQ
					nCurIdx2 = *(WORD*)(pTmp + low * 4);
#else
					nCurIdx2 = *(WORD*)(pTmp + low * 10);
#endif
				}

				break;

			}
			if (nCurIdx2 > yWord)
				high = mid - 1;
			else
				low = mid + 1;
		}

/*		//find the first idx2
		while( low <= high ) 
		{
			mid = (low + high) / 2;
			nCurIdx2 = *(WORD*)(pTmp + mid * 10);

			//wdtmp = (WORD)__min(mid + 1, num - 1);
			//rword = *(WORD*)(pTmp + wdtmp*10);

			if(nCurIdx2 < xWord)
			{
				wdtmp = (WORD)__min(mid + 1, num - 1);
				rword = *(WORD*)(pTmp + wdtmp*10);
				if(rword >= xWord)
					break;
				else
					low = mid + 1; //continue in high interval
			}
			else if(nCurIdx2 == xWord)
				break;
			else
				high = mid - 1; //continue in low interval
		}// end of while
		lowLevel = mid;
		//
		low = mid;
		high = num - 1;
		while(low <= high ) 
		{
			mid = (low + high) / 2;
			nCurIdx2 = *(WORD*)(pTmp + mid * 10);

			if(nCurIdx2 > yWord)
			{
				wdtmp = (WORD)__max(mid-1, 0);
				rword = *(WORD*)(pTmp + wdtmp*10);
				if(rword <= yWord)
					break;
				else
					high = mid - 1;
			}
			else if(nCurIdx2 == yWord)
				break;
			else 
				low = mid + 1; //continue in high interval
		}// end of while
		highLevel = mid;
		//
		index = lowLevel;
		while(index <= highLevel)
		{
			nCurIdx2 = *(WORD*)(pTmp + index * 10);
			if((nCurIdx2 >= xWord) & (nCurIdx2 <= yWord))
			{
				probTmp = *(long*)(pTmp + index * 10 + 2);
				if(probTmp > probLmMax){
					probLmMax = probTmp;
					*maxWord = nCurIdx2;
				} 
			}
			index++;
		}
*/
	}
	if(probLmMax == -19980049)
		probLmMax = NEG_HUGE;

	return probLmMax;
}


void CLM::ResetBuffer()
{

}

/*
BOOL CLM::GetDisFromBuffer(WORD w1, WORD w2, int node, long &curProb)
{
	int i, cacheNum, curHead;
	curHead = pLmCacheHead[node];
	if(lmCacheFull[node])
		cacheNum = MAX_CACHE_LIST;
	else
		cacheNum = curHead;

	for(i = 0; i < cacheNum; i++)
		if(lmCache[node][i].w1 == w1 && lmCache[node][i].w2 == w2)
			break;

	if(i != cacheNum)
	{
		curProb = lmCache[node][i].prob;
		return TRUE;
	}
	else
		return FALSE;

}
*/
/*
BOOL CLM::InsertBuffer(WORD w1, WORD w2, int nGrammarId, long maxProb)
{
	return TRUE;
}

//返回的resIdx是在 m_pKeyHead中的编号
//如果返回TRUE：	找到了，并将找到的结果返回给resIdx
//如果返回FALSE:	没有找到，返回的resIdx为插入点的下一个值
BOOL CLM::SearchInKeyHead(DWORD w1w2, int *resIdx)
{
	int iBegin = 0;
	int iMid = 0;

	int number = LM_BUFFER_HEAD_SIZE;
	if (!m_fBufferFull)
		number = m_iKeyHeadNext;
	int iEnd = number - 1;

	if ((iEnd >= 0) && (m_pLMBuffer[m_pKeyHead[iEnd]].w1w2 < w1w2)){
		*resIdx = iEnd + 1;
		return FALSE;
	}

	while (iBegin <= iEnd){
		iMid = (iBegin + iEnd) / 2;

		if (m_pLMBuffer[m_pKeyHead[iMid]].w1w2 == w1w2){
			*resIdx = iMid;
			return TRUE;
		}
		else if (m_pLMBuffer[m_pKeyHead[iMid]].w1w2 < w1w2){
			if (m_pLMBuffer[m_pKeyHead[iMid + 1]].w1w2 > w1w2){
				iMid++;
				break;
			}
			else
				iBegin = iMid + 1;
		}
		else 
			iEnd = iMid - 1;
	}

	*resIdx = iMid;
	return FALSE;
}
*/

#ifdef USE_LM_PREDICT

int CLM::FindLmCache2(int indextmp, unsigned short word1, unsigned short word2, long &prob)
{
/////version1//
	int cycleNum, kk;

	if(lmCacheFull2[indextmp])
		cycleNum = MODE_VALUE_SECOND;
	else 
		cycleNum = pLmCacheHead2[indextmp];

	for(kk = 0; kk < cycleNum; kk++)
	{
#ifdef DEBUG_LM_CACHE
		intertotal++;
#endif
		if(lmCache2[indextmp][kk].w1 == word1 && lmCache2[indextmp][kk].w2 == word2)
			break;
	}
	if(kk < cycleNum)
	{
		prob = lmCache2[indextmp][kk].prob;
#ifdef DEBUG_LM_CACHE
		interfind++;
#endif
		return 1;
	}
	else
		return 0;
/*
	int cycleNum;

	if(lmCacheFull2[indextmp])
		cycleNum = MODE_VALUE_SECOND;
	else 
		cycleNum = pLmCacheHead2[indextmp];
	int low = 0;
	int high = cycleNum;
	while(low <= high)
	{
		int mid = (low + high) / 2;
		//if(lmCache2[indextmp][kk].w1 == word1 && lmCache2[indextmp][kk].w2 == word2)
		int comres = CompareValue(lmCache2[indextmp][mid].w1, lmCache2[indextmp][mid].w2, word1, word2);
		if(comres == 0)
		{
			prob = lmCache2[indextmp][mid].prob;
			return 1;
		}
		//else if(lmCache2[indextmp][kk].w2 > word2 || (lmCache2[indextmp][kk].w2 == word2 && lmCache2[indextmp][kk].w1 > word1))
		else if(comres > 0)
		{
			high = mid - 1;
		}
		else if(comres < 0)//lmCache2[indextmp][kk].w2 < word2 || (lmCache2[indextmp][kk].w2 == word2 && lmCache2[indextmp][kk].w1 < word1))
		{
			low = mid + 1;
		}
	}
	return 0;
	*/
}

void CLM::InsertLmCache2(int indextmp, unsigned short word1, unsigned short word2, long prob)
{
/////version1//
	int cycleNum, kk;

#ifdef DEBUG_LM_CACHE
		interlmct++;
#endif

	if(lmCacheFull2[indextmp])
	{
		pLmCacheHead2[indextmp] %= MODE_VALUE_SECOND;
		kk = pLmCacheHead2[indextmp];
		lmCache2[indextmp][kk].w1 = word1;
		lmCache2[indextmp][kk].w2 = word2;
		lmCache2[indextmp][kk].prob = prob;
		pLmCacheHead2[indextmp]++;
	}
	else
	{
		kk = pLmCacheHead2[indextmp];
		lmCache2[indextmp][kk].w1 = word1;
		lmCache2[indextmp][kk].w2 = word2;
		lmCache2[indextmp][kk].prob = prob;
		pLmCacheHead2[indextmp]++;
		if(pLmCacheHead2[indextmp] == MODE_VALUE_SECOND)
			lmCacheFull2[indextmp] = 1;
	}
	/*

	if(lmCacheFull2[indextmp])
		cycleNum = MODE_VALUE_SECOND;
	else 
		cycleNum = pLmCacheHead2[indextmp];
	int low = 0;
	int high = cycleNum;
	while(low <= high)
	{
		int mid = (low + high) / 2;
		//if(lmCache2[indextmp][kk].w1 == word1 && lmCache2[indextmp][kk].w2 == word2)
		int comres = CompareValue(lmCache2[indextmp][mid].w1, lmCache2[indextmp][mid].w2, word1, word2);
		if(comres == 0)
		{
			prob = lmCache2[indextmp][mid].prob;
			return 1;
		}
		//else if(lmCache2[indextmp][kk].w2 > word2 || (lmCache2[indextmp][kk].w2 == word2 && lmCache2[indextmp][kk].w1 > word1))
		else if(comres > 0)
		{
			high = mid - 1;
		}
		else if(comres < 0)//lmCache2[indextmp][kk].w2 < word2 || (lmCache2[indextmp][kk].w2 == word2 && lmCache2[indextmp][kk].w1 < word1))
		{
			low = mid + 1;
		}
	}
	return 0;*/
}
/*
int CompareValue(unsigned short refw1, unsigned short refw2, unsigned short w1, unsigned short w2)
{
	int tmp;
	tmp = refw2 - w2;
	if(tmp)
		return tmp;
	tmp = refw1 - w1;
	if(tmp)
		return tmp;
	return 0;
}*/
#endif


#ifdef USE_INDEX_COMPRESS
int CLM::FindTrigramIdxOffset(int offset, int lowtmp, int hightmp)
{
	int low = lowtmp;//0;
	int high = hightmp;//m_trigramIdxOffsetNum - 1;
	int mid;
	unsigned char bFound = 0;

	while( low <= high ) 
	{
		mid = (low + high) / 2;

		if (m_pTrigramIdxOffset[mid] == offset)
		{
			if(m_pTrigramIdxOffset[mid+1] > offset)
			{
				mid = mid + 1;
				bFound = 1;
				break;
			}
			low = mid + 1;
		}
		else if (m_pTrigramIdxOffset[mid] > offset) 
		{
			high = mid-1; //continue in low interval
			if(m_pTrigramIdxOffset[high] <= offset)
			{
				bFound = 1;
				break;
			}
		}
		else 
		{
			low = mid + 1; //continue in high interval
			if(m_pTrigramIdxOffset[low] > offset)
			{
				bFound = 1;
				mid = mid + 1;
				break;
			}
		}
	}
	if(bFound)
		return mid;
	else
		return -1;
}
#endif

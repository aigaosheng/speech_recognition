// LM.h: interface for the CLM class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LM_H__BC0AA61C_037F_4A29_B8C3_12619F51E5FA__INCLUDED_)
#define AFX_LM_H__BC0AA61C_037F_4A29_B8C3_12619F51E5FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//////////宏定义/////////////////
#define USE_LM_PREDICT  //定义后使用语言模型预测
#define	USE_LM_VQ  //定义后使用语言模型概率压缩
#define	CODEWORD_SIZE	unsigned char
#define	TRIGRAM_SECOND_INDEX
#define USE_INDEX_COMPRESS  //定义后使用语言模型索引压缩
/////////////////////////////////


#include	"modeManager.h"

#define LM_BUFFER_HEAD_SIZE 100// <= 65536
#define LM_BUFFER_NODE_RESIZE 100

typedef struct{
	unsigned int word, node;
	//unsigned short w1, w2;
	//unsigned int node;
	long prob;
//	short left, right;
}CDisLmCache;
typedef struct{
	unsigned short w1, w2;
	long prob;
}CDisLmCache1;

class CLM  
{
public:
	void Init();
	short LoadData(short);
	void UnLoadData(short mode);
	long GetLMScore(WORD w1, WORD w2, WORD w3, long);
	long GetLMPredictScore(WORD w1, WORD w2, int nGrammarNode, 
					WORD wXWord, WORD wYWord, WORD lMaxUnigram, long);
	long CheckTriBackoff(WORD w1, WORD w2);
	CLM();
	virtual ~CLM();

private:
	void CloseFileHandle(short);
	LPBYTE ReadFileOnlyBuffer(LPCTSTR lpszKey, HANDLE &hFile, HANDLE &hFileMap);
public:
	long GetDisBigram(WORD w1, WORD xWord, WORD yWord);
	long GetDisTrigram(WORD w1, WORD w2, WORD xWord, WORD yWord);
	//inline long CheckTriBackoff(WORD w1, WORD w2);
	inline long CheckBigram(WORD w1, WORD w2);
	inline long CheckTrigram(WORD w1, WORD w2, WORD w3);
	LPBYTE ReadFileNoBuffer(LPCTSTR lpszKey);
 
	HANDLE m_hFileMapTrigram[MAX_MODE_NUM];
	HANDLE m_hFileMapBigram[MAX_MODE_NUM];
	HANDLE m_hFileTrigram[MAX_MODE_NUM];
	HANDLE m_hFileBigram[MAX_MODE_NUM];
	LPBYTE m_pTrigramMode[MAX_MODE_NUM];
	LPBYTE m_pTrigramIdxMode[MAX_MODE_NUM][2];
	LPBYTE m_pBigramMode[MAX_MODE_NUM];
	LPBYTE m_pBigramIdxMode[MAX_MODE_NUM];
	LPBYTE m_pUnigramMode[MAX_MODE_NUM];
	LPBYTE m_pTrigram;
	LPBYTE m_pTrigramIdx[2];
	LPBYTE m_pBigram;
	LPBYTE m_pBigramIdx;
	LPBYTE m_pUnigram;

	//以下部分为与语言模型Buffer相关的部分
/*	BOOL			m_fBufferFull;
	HANDLE			m_hHeap;
	WORD			m_iKeyHeadNext;
	WORD			m_pKeyHead[LM_BUFFER_HEAD_SIZE];
	LM_BUFFER_HEAD	m_pLMBuffer[LM_BUFFER_HEAD_SIZE];

	BOOL GetDisFromBuffer(WORD w1, WORD w2, int nGrammerId, long &Prob);
	BOOL InsertBuffer(WORD w1, WORD w2, int nGrammerId, long maxProb);
	BOOL SearchInKeyHead(DWORD w1w2, int *resIdx);
*/	//
	//语言模型预测概率的cache
#ifdef USE_LM_PREDICT

	CDisLmCache	lmCache[MODE_VALUE_LM_CACHE][MAX_CACHE_LIST];
	short		pLmCacheHead[MODE_VALUE_LM_CACHE];
	char		lmCacheFull[MODE_VALUE_LM_CACHE];
	//short		pLmCacheFreq[MODE_VALUE_LM_CACHE];

	CDisLmCache1	lmCache2[MODE_VALUE_LM_CACHE_SECOND][MODE_VALUE_SECOND];
	short		pLmCacheHead2[MODE_VALUE_LM_CACHE_SECOND];
	char		lmCacheFull2[MODE_VALUE_LM_CACHE_SECOND];
#endif

#ifdef USE_LM_VQ
	long* m_pLmCodebook;
#endif

#ifdef USE_INDEX_COMPRESS
	unsigned int* m_pTrigramIdxOffsetMode[MAX_MODE_NUM];
	unsigned int* m_pTrigramIdxOffset;
	int	m_trigramIdxOffsetNum;
	int	FindTrigramIdxOffset(int, int, int);
#endif

public:
	void ResetBuffer();
	
#ifdef USE_LM_PREDICT
	  int  FindLmCache2(int indextmp, unsigned short word1, unsigned short word2, long& prob);
	  void InsertLmCache2(int indextmp, unsigned short word1, unsigned short word2, long prob);
#endif

};

#endif // !defined(AFX_LM_H__BC0AA61C_037F_4A29_B8C3_12619F51E5FA__INCLUDED_)

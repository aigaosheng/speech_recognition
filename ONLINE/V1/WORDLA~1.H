// WordLattice.h: interface for the CWordLattice class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WORDLATTICE_H__D629C075_1D90_4644_BAA9_EB56DD0A4A4C__INCLUDED_)
#define AFX_WORDLATTICE_H__D629C075_1D90_4644_BAA9_EB56DD0A4A4C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef struct tagPartialTrace
{
	short	m_nSeqLen;
	short	m_timeSeq[MAX_SENTENCE_PARTIAL_LENGTH];
	unsigned short	m_wordSeq[MAX_SENTENCE_PARTIAL_LENGTH];
	long	m_scoreAm[MAX_SENTENCE_PARTIAL_LENGTH], m_scoreLm[MAX_SENTENCE_PARTIAL_LENGTH];
}S_PARTIAL_RESULT;

class CWordLattice  
{
//Construction/Destruction:
public:
	CWordLattice();
	virtual ~CWordLattice();
	void Init(CLM *lm);
	void ResetWordCounter(void); //高升编辑

//Attribute:
private:
	//LATTICENODE m_WordLattice[MAX_SPEECH_LENGTH][MAX_WORD_CANDIDATES];
	LATTICENODE m_WordLattice[MAX_WORD_LATTICE_STACK][MAX_WORD_CANDIDATES];
	int m_ValidNum[MAX_WORD_LATTICE_STACK];
	int m_WordSeq[MAX_SENTENCE_LENGTH];
	int m_nLastTime;
	int m_nSeqInc;
	CLM * m_LM;
	//以下是高声编辑
	short m_FreeLatticeStackTop; //管理空闲laiitce的栈顶
	short m_FreeLatticeStack[MAX_WORD_LATTICE_STACK]; //管理空闲laiitice的堆栈
	short m_TimeLatticeMap[MAX_CONTINUOUS_TIME]; //时间和lattice的映射
	short m_PreConfirmTime;
	S_PARTIAL_RESULT m_partialResult[MAX_CONFIRM_BLOCK];
public:
	short m_traceCount;
	short m_traceStartTime, m_partialNum;
	unsigned short m_partialWord[2];
	long	m_refAmScore, m_refLmScore;

//relation:
public:
	void PushLattice(int nTime, int nWordNumber, LPLATTICENODE pLattice);
	void GetWordSequence(int &nWordLength,int * WordSquence);
	int  GeneratePartialWord(void);
//behavior:
public:
	void TraceBack(void);
	int PartialTraceBack(int);	
private:
	void ReleaseWordLattice(int, int);

};

#endif // !defined(AFX_WORDLATTICE_H__D629C075_1D90_4644_BAA9_EB56DD0A4A4C__INCLUDED_)

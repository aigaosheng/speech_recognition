// AM.h: interface for the CAM class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AM_H__1448B168_E2B9_4113_BBF4_43790AE219E5__INCLUDED_)
#define AFX_AM_H__1448B168_E2B9_4113_BBF4_43790AE219E5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define USE_SSE

#include	"struct.h"
class CAM  
{
public:
	void ResetObserveBuffer(float *pVector);
	void ExpandNode(PHONENODE & SeedNode);
	inline int GetHMMMaxState(int nModelID);
	inline int Mapping(int nClassTriphone);
	inline int TriphoneMappingI(short nBasePhone, short nLeftPhone, short nRightPhone);
	inline int TriphoneMappingF(short nBasePhone, short nBaseTone , short nLeftTone , 
		                          short nRightTone, short nLeftPhone, short nRightPhone );
	BOOL Init();

	CAM();
	//
	void ResetAmCacheBuffer(void);
	virtual ~CAM();
	//
	short baseStartTricode[TOTAL_PHONES], maxTriphoneNum;
	short m_nextCodeWord;
	long GetObserveProb(WORD nModel, WORD iState, WORD jState, short);

public:
	int m_nTotalTriphone, m_triSilenceCode, m_nTotalOutput;

private:

#ifdef USE_SSE
	__m128*				m_pVector;
#else
	float				m_pVector[NO_DIM];
#endif

	long*				m_pObserveBuffer;
	HMM*				m_pHmm;
	OUTPUT_ALIGNED*		m_pOutput;
	short*				m_pTriphoneMappingInitial;
	short*				m_pTriphoneMappingFinal;
	BYTE*				m_pMapping;
	DWORD m_time;
	//以下由高升添加
	short				m_codeWord;
	short*				m_pShortListNum;
	int*				m_pShortListOffset;
	char*				m_pShortList;
	float*				m_pClusterMean;
	float*				m_pClusterVar;
	BOOL LoadShortList(void);
	short FindCodebook(float *pVec);
	float nextVector[NO_DIM];
	//
	long GetTranslateProb(WORD nMode, WORD nState1, WORD nState2);
//	long GetObserveProb(WORD nModel, WORD iState, WORD jState, short);
	long GetObserveProbOld(WORD nModel, WORD iState, WORD jState);
	BOOL LoadHmm();
	BOOL LoadOutput();
	BOOL LoadTriphoneMapping();
	BOOL LoadMapping();

};

inline int CAM::Mapping(int nClassTriphone)
{
	return m_pMapping[nClassTriphone];
}

inline int CAM::GetHMMMaxState(int nModelID)
{
	return m_pHmm[nModelID].No_State;
}

const	int	baseOffsetI = 1406;//(TOTAL_FINAL + 1) * TOTAL_FINAL

inline int CAM::TriphoneMappingI(short base, short left, short right)
{
	ASSERT(base != SILENCE_CODE);
	left -= TOTAL_INITIAL;
	right -= TOTAL_INITIAL;
	return *(m_pTriphoneMappingInitial + 
			        base * baseOffsetI + 
					left * TOTAL_FINAL + 
					right);

}

const	int baseOffset = 108000; //TOTAL_INITIAL * (TOTAL_INITIAL + 1) * 5 * 6 * 6
const	int	baseToneOffset = 21600;//TOTAL_INITIAL * (TOTAL_INITIAL + 1) * 6 * 6
const	int	leftToneOffset = 3600;//TOTAL_INITIAL * (TOTAL_INITIAL + 1) * 6
const	int	rightToneOffset	= 600;//TOTAL_INITIAL * (TOTAL_INITIAL + 1)

inline int CAM::TriphoneMappingF(short base, short baseTone, short leftTone, short rightTone, short left,short  right)
{
	base -= TOTAL_INITIAL;
	right %= TOTAL_FINAL;
	return *(m_pTriphoneMappingFinal + 
			base * baseOffset + 
			baseTone * baseToneOffset + 
			leftTone * leftToneOffset + 
			rightTone * rightToneOffset +
			left * (TOTAL_INITIAL + 1) + 
			right);
	/*
	return *(m_pTriphoneMappingFinal + 
			base * TOTAL_INITIAL * (TOTAL_INITIAL + 1) * 5 * 6 * 6 + 
			baseTone * TOTAL_INITIAL * (TOTAL_INITIAL + 1) * 6 * 6 + 
			leftTone * TOTAL_INITIAL * (TOTAL_INITIAL + 1) * 6 + 
			rightTone * TOTAL_INITIAL * (TOTAL_INITIAL + 1) +
			left * (TOTAL_INITIAL + 1) + 
			right);
*/
}

#endif // !defined(AFX_AM_H__1448B168_E2B9_4113_BBF4_43790AE219E5__INCLUDED_)

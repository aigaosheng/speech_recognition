// Path.h: interface for the CPath class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PATH_H__B5E12EB0_7235_427E_AD2F_58210B5DCAD0__INCLUDED_)
#define AFX_PATH_H__B5E12EB0_7235_427E_AD2F_58210B5DCAD0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "const.h"

class CPath  
{
private:	
	
	int freeBlockTop;
	int freeBlockIdx[NSLMAX];	
	
public:
	PHONENODE pTokenPath[NSLMAX];
	int pBucketHead[2][MODULE_VALUE1+MODULE_VALUE2];	
	int flag1;
	int flag2;
	long m_lProbMax;
	//当前路径管理的堆栈
	BIDIRECT_KEY_LIST	keyList[2][MODULE_VALUE1+MODULE_VALUE2];
	int keyHead[2][2], keyTail[2][2];
	//
public:
	CPath();
	virtual ~CPath();

public:
	int InsertPath(int stackIdx, PHONENODE path, int type);
	
	void PushInterHMMNode(int nNodes, LPPHONENODE pNodes);
	void PushInitHMMNode(int nNodes,  LPPHONENODE pNodes);
	
	void BeamPruning(int startNo, int moduleValue, int pathThreshold, const long *M_EPS);	
	
	void ReleaseBlock(int flag, int keyCt, int preIndex, int CurIndex, int beamLevel);
		
	void Init( );

	void PushIntraHMMNode(int nNodes, LPPHONENODE pNodes);	
	void PushInterWordNode(int nNodes, LPPHONENODE pNodes);	
	
	void ExchangeStack( );
private:
	inline void InsertBidirectKeylist(int flag, int beamLevel, int key);
	inline void RemoveBidirectKeylist(int flag, int beamLevel, int keyCt);
};

const long M_EPS_INTRA[BLOCKNUM]={400000L, 600000L, 800000L, 900000L, 1000000L};

const long M_EPS_INTER[BLOCKNUM]={600000L, 650000L, 700000L, 800000L, 1000000L};//900000L};//

const long maxOffsetAm = 1000000L;
const long maxOffsetLm = 1000000L;//900000L;//

#endif // !defined(AFX_PATH_H__B5E12EB0_7235_427E_AD2F_58210B5DCAD0__INCLUDED_)

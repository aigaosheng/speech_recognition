// Lextree.h: interface for the CLextree class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LEXTREE_H__3A27B961_B910_4EAE_AA33_73DF363D450D__INCLUDED_)
#define AFX_LEXTREE_H__3A27B961_B910_4EAE_AA33_73DF363D450D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include	"modeManager.h"
/////////////////////////////////////////////////////////////////////

class CLextree  
{
//Construction/Destruction:
public:
	CLextree();
	void Init(CLM *lm,CAM *am);

	virtual ~CLextree();

//Attribute:
public:
//private:
	//int m_nNeedExtendWord;
	int m_nWordNumber;
	//int m_nMergeInc;
	//int m_nNodeInitInc;
	//int m_nRootLinkNo;
	//int m_nRootPhone;
	//int m_nNeedExtendNode;
	//int m_nMaxNode;
//	short m_nTime;
	//int m_nFatherBasePhone;
	//int m_nFatherLeftBasePhone;
	//int m_nFatherNodeNo;
	//int m_nFatherPhoneIdx;
	//char m_cFatherTone;
	//char m_cFatherLeftTone;

//	WORD m_wWord1;
//	WORD m_wWord2;
//	long m_lLMScore; 
//	long m_lAMScore; 

	LATTICENODE m_WordList[MAX_WORD_CANDIDATES];
	int m_WordListProbTaxis[MAX_WORD_CANDIDATES];
	LPLEXTREE	m_lpLextree,  //指向当前激活的模式词典
				m_lpLextreeMode[MAX_MODE_NUM]; //存贮对应模式的词典
	//PHONENODE m_fatherNode;
	CLM *m_LM;
	CAM *m_AM;

//method:
//private:
public:
	void LoadLextree(short loadMode);
	void UnLoadLextree(short loadMode);
	
//relation:
public:
	//int GetInitPathNode(int &nNode,LPPHONENODE pNode);
	void GetWordLatticeList(LPLATTICENODE);
	//void GetWord(int nIndex,LATTICENODE &wrod);
	int GetWordNumber();
	void BeginInitPathNode(void);
	BOOL InterWordExtend(int);
	void IntraWordExtend(PHONENODE &node,int nTime);//, int &nNode,LPPHONENODE pNode,int nTime);
	//int GetIntraWordExtendNodes(int &nNode,LPPHONENODE pNode);
	//int GetInterWordExtendNodes(int &nNode,LPPHONENODE pNode);

//behavior:
private:
	//void MergePathNode(PHONENODE nodeTmp,int &nNode,LPPHONENODE pNode);
	void TreeLeafExtend(PHONENODE &node,int nNeedExtendNode, long backweight);
	void FlyingExtendFinal(PHONENODE &node,int nNeedExtendNode,long backweight);
	void FlyingExtendInitial(PHONENODE &node,int nNeedExtendNode,long backweight);
	void CollectWord(PHONENODE &node,int nEndNode,int nTime);
//
};

#endif // !defined(AFX_LEXTREE_H__3A27B961_B910_4EAE_AA33_73DF363D450D__INCLUDED_)

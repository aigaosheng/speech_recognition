// Lextree.cpp: implementation of the CLextree class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "const.h"
#include "global.h"
#include "lm.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define		PENALTY 0//-5000//

//#define	USE_AM_PREDICT

CLextree::CLextree()
{
	m_AM = NULL;
	m_LM = NULL;
	m_lpLextree = NULL;
	m_nWordNumber = 0;
	//m_nMergeInc = 0;
//	m_nNodeInitInc = 0;
//	m_nRootLinkNo = -1;
//	m_nRootPhone = -1;
//	m_nNeedExtendNode = 0;
//	m_nMaxNode = 0;
//	m_nTime = -1;
//	m_nNeedExtendWord = 0;
//	m_nFatherBasePhone = -1;
//	m_nFatherLeftBasePhone = -1;
//	m_nFatherNodeNo = -1;
//	m_nFatherPhoneIdx = -1;
//	m_cFatherTone = -1;
//	m_cFatherLeftTone = -1; 
//	m_wWord1 = (WORD)0;
//	m_wWord2 = (WORD)0;;
//	m_lLMScore = -1;
//	m_lAMScore = -1;
}

CLextree::~CLextree()
{
/*
	if (m_lpLextree)
		VirtualFree(m_lpLextree,0,MEM_RELEASE);
*/
	for(int i = 0; i < MAX_MODE_NUM; i++)
	{
		if(g_pModeManager->CheckModeLoaded(i))
			UnLoadLextree(i);
	}
}

void CLextree::IntraWordExtend(PHONENODE &node,int nTime)//, int &nNode, LPPHONENODE pNode,int nTime)
{
	int phnidx, jj, fatherContext, leftBasePhone, rightBasePhone, isCollectWord, 
		keyCt, curIndex, preIndex, linkNo, basePhone, temp, jj_g;
	unsigned short i_s, word1, word2;
	short leftPhone;
	long   probTmp, triBackWeight, i_g, j_g, stNode;
	char leftTone, leftBaseTone, rightBaseTone;

	i_g = node.grammarNode;
	i_s = node.tokenActive;
	phnidx = node.modelId;

	leftPhone = m_lpLextree[i_g].nodeId; 
	leftTone = m_lpLextree[i_g].toneId; 
	ASSERT(leftPhone == g_pAM->Mapping(phnidx));
	//
	word1 = node.historyWord1;
	word2 = node.historyWord2;
	//
	leftBasePhone = g_pAM->Mapping(node.leftTriphone);
	leftBaseTone = node.leftTone; //get left tone of this model
	//
	triBackWeight = m_LM->CheckTriBackoff(word1, word2);
	//if(triBackWeight == NEG_HUGE)
	//	triBackWeight = 0;
	ASSERT(triBackWeight != NEG_HUGE);

	//find the successors of current node
	linkNo = m_lpLextree[i_g].linkNum;
	stNode = m_lpLextree[i_g].startNode;
	//check the type of node
	if(stNode >= 0) 
	{
		//intra-word transition. tree node of current path is not a leaf
		ASSERT(stNode != 0);
		for(jj_g = 0; jj_g < linkNo; jj_g++)
		{
			//extend all successors of current grammar node i_g
			j_g = stNode + jj_g; //extended successor node
			//hmmModelCode = m_lpLextree[j_g].nodeId;
			//rightBasePhone = mappingPitch[hmmModelCode];
			rightBasePhone= m_lpLextree[j_g].nodeId;
			rightBaseTone = m_lpLextree[j_g].toneId;
			//check type of extended node
			if(rightBasePhone <= MAX_CODE_INITIAL_PART) 
			{
				if(leftPhone != SILENCE_CODE)
 					if(m_AM->TriphoneMappingF(leftPhone, leftTone, leftBaseTone, rightBaseTone, leftBasePhone, rightBasePhone) != phnidx)			
						continue;
				FlyingExtendInitial(node, j_g, triBackWeight); //stack index1, stack index2, grammar node(base phone), right grammar
			}
			else if(rightBasePhone <= MAX_CODE_FINAL_PART)
			{
				//current node j_g is virtual model code and must dynamically extend all its possible triphones.
				//tree node ID is tonal base phone. fan-out all possible triphone with the same base phone
				//the Virtual node is FINAL 
				ASSERT(leftPhone <= MAX_CODE_INITIAL_PART);
				ASSERT(leftBasePhone > MAX_CODE_INITIAL_PART);
				ASSERT(rightBasePhone > MAX_CODE_INITIAL_PART && rightBasePhone <= MAX_CODE_FINAL_PART);
				//Check the right context corresponding to node 'j_g'
				if(m_AM->TriphoneMappingI(leftPhone, leftBasePhone, rightBasePhone) != phnidx)
					continue;
				FlyingExtendFinal(node, j_g, triBackWeight); //stack index1, grammar node(base phone), backoff weight
			}
			else
			{
				//reach the last state of last phone of a word and word can be determined. Collect word
				isCollectWord = 0;
				ASSERT(leftPhone > MAX_CODE_INITIAL_PART && leftPhone <= MAX_CODE_FINAL_PART);
				ASSERT(leftBasePhone <= MAX_CODE_INITIAL_PART);
				ASSERT(rightBasePhone == SILENCE_CODE);
				probTmp = m_LM->GetLMPredictScore(word1, word2, j_g, m_lpLextree[j_g].wordFirst, m_lpLextree[j_g].wordLast, m_lpLextree[j_g].maxUnigramProb, triBackWeight);
				for(jj = 0; jj <= PITCH_NULL; jj++)
				{
					if(m_AM->TriphoneMappingF(leftPhone, leftTone, leftBaseTone, jj, leftBasePhone, rightBasePhone) == phnidx)							
					{
//						probTmp = m_LM->GetLMPredictScore(word1, word2, j_g, m_lpLextree[j_g].wordFirst, m_lpLextree[j_g].wordLast, m_lpLextree[j_g].maxUnigramProb, triBackWeight);
						TreeLeafExtend(node, j_g, probTmp); //silence is in the context set of current path. then extend current path to j_g node
						isCollectWord++;
						break;
					}
				}
				//
				isCollectWord = 0;
				fatherContext = m_lpLextree[0].startNode;
				for(jj = 0; jj < m_lpLextree[0].linkNum; jj++)
				{
					basePhone = m_lpLextree[fatherContext+jj].nodeId;
					temp = m_lpLextree[fatherContext+jj].toneId;
					if(m_AM->TriphoneMappingF(leftPhone, leftTone, leftBaseTone, temp, leftBasePhone, basePhone) == phnidx)
					{
						CollectWord(node, j_g, nTime);
						break;
					}
				}
			}
		}
	}
	else
	{
		//readch the leaf of a word. tree node of current path is a leaf.
		ASSERT(m_lpLextree[i_g].nodeId == SILENCE_CODE);
		CollectWord(node, i_g, nTime);
	}
}

BOOL CLextree::InterWordExtend(int curTime)//short nTime,short nWordIdx,int &nNode,LPPHONENODE pNode)
{
	//declare local variables
	int jj, fatherContext, leftBasePhone, linkNo, nextLinkNo, hmmModelCode2, basePhone, ii_g, jj_g;
	unsigned short word1, word2;
	long probAm, probLm, triBackWeight, i_g, j_g, stNode, nextStNode, probSon;
	char leftTone, leftBaseTone, rightBaseTone;
	short leftPhone, rightPhone;
	PHONENODE tokenPathTmp;

	//inter-word extension and extend word in word-lattice to the first level of lexical tree
	linkNo = m_lpLextree[0].linkNum;
	stNode = m_lpLextree[0].startNode;

	for(ii_g = 0; ii_g < m_nWordNumber; ii_g++)
	{
		fatherContext = m_WordList[ii_g].leftTriphone;
		leftBasePhone = m_AM->Mapping(fatherContext);
		leftBaseTone = m_WordList[ii_g].leftTone; //get left tone
		//
		hmmModelCode2 = m_WordList[ii_g].lastModel;
		leftPhone = m_AM->Mapping(hmmModelCode2);
		leftTone = m_WordList[ii_g].toneId; //get self-tone
		//
		if(m_WordList[ii_g].preTime < 0)
			word1 = m_nHisWord2; //高生编辑
		else
			word1 = m_WordList[ii_g].hisWord2;

		word2 = m_WordList[ii_g].wordId;
		probAm = m_WordList[ii_g].amScore;
		probLm = m_WordList[ii_g].lmScore;
		//	
		if(ACOUSTIC_WEIGHT * probAm + LANGUAGE_WEIGHT * probLm < g_pPath->m_lProbMax - maxOffsetLm)
			continue;

		triBackWeight = m_LM->CheckTriBackoff(word1, word2);
		if(triBackWeight == NEG_HUGE)
			triBackWeight = 0;

		for(jj = 0; jj < linkNo; jj++)
		{
			i_g = jj + stNode;
			//basePhone = mappingPitch[m_lpLextree[i_g].nodeId];
			basePhone = m_lpLextree[i_g].nodeId;
			//find all possible right conext
			nextLinkNo = m_lpLextree[i_g].linkNum;
			nextStNode = m_lpLextree[i_g].startNode;

			if(leftPhone != SILENCE_CODE)
			{
				//left phone must be FINAL
				ASSERT(leftPhone > MAX_CODE_INITIAL_PART);
				rightBaseTone = m_lpLextree[i_g].toneId;
				if(m_AM->TriphoneMappingF(leftPhone, leftTone, leftBaseTone, rightBaseTone, leftBasePhone, basePhone) != hmmModelCode2)
					continue;
			}

			for(jj_g = 0; jj_g < nextLinkNo; jj_g++)
			{
				j_g = nextStNode + jj_g;
				//rightPhone = mappingPitch[m_lpLextree[j_g].nodeId];
				rightPhone = m_lpLextree[j_g].nodeId;
				ASSERT(rightPhone > MAX_CODE_INITIAL_PART && rightPhone != SILENCE_CODE);

				ASSERT(basePhone <= MAX_CODE_INITIAL_PART);
				//dynamically calculate the predictive lM score
//
#ifdef USE_LM_PREDICT
int indextmp = j_g - (m_lpLextree[0].linkNum + 1);
if(!m_LM->FindLmCache2(indextmp, word1, word2, probSon))
{
	probSon = m_LM->GetLMPredictScore(word1, word2, j_g, m_lpLextree[j_g].wordFirst, m_lpLextree[j_g].wordLast, m_lpLextree[j_g].maxUnigramProb, triBackWeight);
	m_LM->InsertLmCache2(indextmp, word1, word2, probSon);
}
#else
	probSon = m_LM->GetLMPredictScore(word1, word2, j_g, m_lpLextree[j_g].wordFirst, m_lpLextree[j_g].wordLast, m_lpLextree[j_g].maxUnigramProb, triBackWeight);
#endif
			//new path
				tokenPathTmp.amScore = probAm;
				tokenPathTmp.lmScore = probLm + probSon + PENALTY; 
				tokenPathTmp.grammarNode = i_g; 
				tokenPathTmp.historyWord1 = word1;
				tokenPathTmp.historyWord2 = word2;
				tokenPathTmp.lmLookaheadScore = probSon;
				tokenPathTmp.modelId = m_AM->TriphoneMappingI(basePhone, leftPhone, rightPhone); 
				tokenPathTmp.preWordPos = ii_g;
				tokenPathTmp.startTime = curTime + 1;
				tokenPathTmp.wordDuration = 0;
				tokenPathTmp.tokenActive = 0;
				tokenPathTmp.leftTriphone = hmmModelCode2;
				tokenPathTmp.nextToken = -1;
				//
				if(leftPhone == SILENCE_CODE && m_WordList[ii_g].modelDur >= 4)
					tokenPathTmp.leftTone = PITCH_NULL;
				else
					tokenPathTmp.leftTone = leftTone;
				//
				long sumScore = ACOUSTIC_WEIGHT * tokenPathTmp.amScore + LANGUAGE_WEIGHT * tokenPathTmp.lmScore;
				if(sumScore	>= g_pPath->m_lProbMax - maxOffsetLm)
				{
#ifdef USE_AM_PREDICT
					//Nov. 3,2000
					long deltaAm = 0;
					if(g_pAM->m_nextCodeWord >= 0)
						deltaAm = g_pAM->GetObserveProb(tokenPathTmp.modelId, 0, 1, g_pAM->m_nextCodeWord);
					if(sumScore + ACOUSTIC_WEIGHT * deltaAm >= g_pPath->m_lProbMax - maxOffsetLm + 70000)
#endif
					{
//						tokenPathTmp.deltaScore = deltaAm;
						g_pPath->InsertPath(g_pPath->flag2, tokenPathTmp, 2);
					}
				}
			}
		}
	}
	return 0;
}

void CLextree::BeginInitPathNode()
{
	PHONENODE	nodeTmp;
	int nStNode,nLinkNo,nBasePhone,nRightPhone,nTriphone,nSecondLevel;

	//Nov 2,2000
	long triBackWeight = m_LM->CheckTriBackoff(m_nHisWord1, m_nHisWord2);
	long curProb = m_LM->GetLMPredictScore(m_nHisWord1, m_nHisWord2, 0, m_lpLextree[0].wordFirst, m_lpLextree[0].wordLast, m_lpLextree[0].maxUnigramProb, triBackWeight);
	//得到根结点
	nodeTmp.grammarNode = 0;
	nodeTmp.leftTriphone = g_pAM->m_triSilenceCode;//TRI_SILENCE_CODE;
	nodeTmp.leftTone = PITCH_NULL;
	nodeTmp.modelId = g_pAM->m_triSilenceCode;//TRI_SILENCE_CODE;
	nodeTmp.tokenActive = 0;
	nodeTmp.amScore = 0;
	nodeTmp.lmScore = curProb;//0; 
	nodeTmp.lmLookaheadScore = curProb;//0; //Nov.2,2000
	//nodeTmp.historyWord1 = HEAD_WORD;
	//nodeTmp.historyWord2 = HEAD_WORD;
	nodeTmp.historyWord1 = m_nHisWord1; //高生编辑
	nodeTmp.historyWord2 = m_nHisWord2; //高生编辑
	nodeTmp.preWordPos = -1;
	nodeTmp.startTime = 0;
	nodeTmp.wordDuration = 0; 
	nodeTmp.nextToken = -1;
	nodeTmp.probLevel = 0;
	//
//	nodeTmp.deltaScore = 0;
	g_pPath->InsertPath(g_pPath->flag1, nodeTmp, 0);

	//Init信息初始化
	int linkNo = m_lpLextree[0].linkNum;
	int leftPhone = m_lpLextree[0].nodeId;
	//
	for(int jj = 0; jj < linkNo; jj++)
	{
		int node = m_lpLextree[0].startNode + jj;
		//以所有的可能扩展此结点(即用所有可能的rightContext进行扩展)
		nLinkNo = m_lpLextree[node].linkNum; 
		nStNode = m_lpLextree[node].startNode;
		nBasePhone = m_lpLextree[node].nodeId;
		//找出相应的所有RightContext
		for(int j = 0; j < nLinkNo; j++)
		{
			nSecondLevel = nStNode + j; 
			//Nov.2,2000
			curProb = m_LM->GetLMPredictScore(m_nHisWord1, m_nHisWord2, nSecondLevel, m_lpLextree[nSecondLevel].wordFirst, m_lpLextree[nSecondLevel].wordLast, m_lpLextree[nSecondLevel].maxUnigramProb, triBackWeight);
			//
			//映射Triphone
			nRightPhone = m_lpLextree[nSecondLevel].nodeId;
			nTriphone = m_AM->TriphoneMappingI(nBasePhone, leftPhone, nRightPhone);
			ASSERT(nTriphone >= 0);
			//生成新结点	
			nodeTmp.grammarNode = node;
			nodeTmp.leftTriphone = g_pAM->m_triSilenceCode;//TRI_SILENCE_CODE;
			nodeTmp.modelId = (short)nTriphone;
//			nodeTmp.historyWord1 = HEAD_WORD;
//			nodeTmp.historyWord2 = HEAD_WORD;
			ASSERT(m_nHisWord1 == HEAD_WORD);
			ASSERT(m_nHisWord2 == HEAD_WORD);
			nodeTmp.historyWord1 = m_nHisWord1; //高生编辑;
			nodeTmp.historyWord2 = m_nHisWord2; //高生编辑;
			nodeTmp.amScore = 0;
			nodeTmp.lmScore = curProb;//0;
			nodeTmp.lmLookaheadScore = curProb;//0;
			nodeTmp.preWordPos = -1;
			nodeTmp.wordDuration = 0;
			nodeTmp.startTime = 0;
			nodeTmp.tokenActive = 0;
			nodeTmp.leftTone = PITCH_NULL;
			nodeTmp.nextToken = -1;
			nodeTmp.probLevel = 0;
//			nodeTmp.deltaScore = 0;
			g_pPath->InsertPath(g_pPath->flag1, nodeTmp, 0);
		}		
	}
}

int CLextree::GetWordNumber()
{
	return m_nWordNumber;
}


void CLextree::GetWordLatticeList(LPLATTICENODE wordList)
{
	memcpy(wordList,m_WordList,m_nWordNumber*sizeof(LATTICENODE));

	memset(m_WordList,0,m_nWordNumber*sizeof(LATTICENODE));
	memset(m_WordListProbTaxis,-1,m_nWordNumber*sizeof(int));
	m_nWordNumber = 0;
}

void CLextree::Init(CLM *lm,CAM *am)
{
	m_LM = lm;
	m_AM = am;
	memset(m_WordList,0,MAX_WORD_CANDIDATES*sizeof(LATTICENODE));
	memset(m_WordListProbTaxis,-1,MAX_WORD_CANDIDATES*sizeof(int));
//	LoadLextree(); //Jan.10,2001
}

/*
void CLextree::FlyingExtendFinal(PHONENODE& node, int baseGrammar, long backWeight)
{
	int linkNo, stNode, linkNoTmp, stNodeTmp, basePhone, leftPhone, leftContext, rightPhone, rightContext, iii, jj, kk1;
	long curProb;
	unsigned short word1, word2;
	char basePitch, leftPitch, rightPitch;
	PHONENODE tokenPathTmp;

	//the phone ID currently extended
	basePhone = m_lpLextree[baseGrammar].nodeId; //current phone extended
	basePitch = m_lpLextree[baseGrammar].toneId; //current pitch 
	ASSERT(basePhone > MAX_CODE_INITIAL_PART && basePhone != SILENCE_CODE);
	//
	leftContext = node.grammarNode; 
	leftPhone = m_lpLextree[leftContext].nodeId; //left context phone of current phone
	leftPitch = node.leftTone; //left context pitch 
	ASSERT(leftPhone <= MAX_CODE_INITIAL_PART);

	word1 = node.historyWord1;
	word2 = node.historyWord2;
	//insert new path into path stack
	tokenPathTmp.amScore = node.amScore;
	tokenPathTmp.grammarNode = baseGrammar;
	tokenPathTmp.historyWord1 = word1;
	tokenPathTmp.historyWord2 = word2;
	tokenPathTmp.preWordPos = node.preWordPos;
	tokenPathTmp.startTime = node.startTime;
	tokenPathTmp.tokenActive = 0;
	tokenPathTmp.wordDuration = 0;
	tokenPathTmp.leftTriphone = node.modelId;
	tokenPathTmp.leftTone = leftPitch; //从INITIAL->FINAL,传递左边的声调
	tokenPathTmp.nextToken = -1;
			
	//find all possible right contexts
	linkNo = m_lpLextree[baseGrammar].linkNum;
	stNode = m_lpLextree[baseGrammar].startNode;
	ASSERT(linkNo < 200);
	//
	//only right phone and right pitch is dynamical
	for(iii = 0; iii < linkNo; iii++)
	{
		rightContext = stNode + iii;
		//dynamically calculate LM predictive probability
		curProb = m_LM->GetLMPredictScore(word1, word2, rightContext, m_lpLextree[rightContext].wordFirst,
		                              m_lpLextree[rightContext].wordLast, m_lpLextree[rightContext].maxUnigramProb, backWeight);

		tokenPathTmp.lmLookaheadScore = curProb;
		tokenPathTmp.lmScore = node.lmScore + (curProb - node.lmLookaheadScore);
		//
		if(ACOUSTIC_WEIGHT * tokenPathTmp.amScore + LANGUAGE_WEIGHT * tokenPathTmp.lmScore >= g_pPath->m_lProbMax - maxOffset)
		{
			if(m_lpLextree[rightContext].startNode > 0)
			{			
				//extend the first final and right context is INITIAL
				rightPhone = m_lpLextree[rightContext].nodeId;;		
				ASSERT(rightPhone <= MAX_CODE_INITIAL_PART);
				rightPitch = m_lpLextree[rightContext].toneId;
				tokenPathTmp.modelId = m_AM->TriphoneMappingF(basePhone, basePitch, leftPitch, rightPitch, leftPhone, rightPhone);

				g_pPath->InsertPath(g_pPath->flag2, tokenPathTmp, 1);
			}
			else
			{	
				ASSERT(m_lpLextree[rightContext].nodeId == SILENCE_CODE);
				rightPhone = m_lpLextree[rightContext].nodeId;
				ASSERT(rightPhone == SILENCE_CODE);
				for(jj = 0; jj <= PITCH_NULL; jj++)
				{
					tokenPathTmp.modelId = m_AM->TriphoneMappingF(basePhone, basePitch, leftPitch, jj, leftPhone, rightPhone);
					g_pPath->InsertPath(g_pPath->flag2, tokenPathTmp, 1);
				}
				//right context is the first INITIAL of the next word
				linkNoTmp = m_lpLextree[0].linkNum;
				stNodeTmp = m_lpLextree[0].startNode;
			
				for(kk1 = 0; kk1 < linkNoTmp; kk1++)
				{
					rightContext = stNodeTmp + kk1;
					rightPhone = m_lpLextree[rightContext].nodeId;
					rightPitch = m_lpLextree[rightContext].toneId;
					ASSERT(rightPhone <= MAX_CODE_INITIAL_PART);
					tokenPathTmp.modelId = m_AM->TriphoneMappingF(basePhone, basePitch, leftPitch, rightPitch, leftPhone, rightPhone);
					//insert new path into path stack
					g_pPath->InsertPath(g_pPath->flag2, tokenPathTmp, 1);
				}
			}
		}
	}
}
*/

void CLextree::FlyingExtendFinal(PHONENODE& node, int baseGrammar, long backWeight)
{
	int linkNo, stNode, linkNoTmp, stNodeTmp, basePhone, leftPhone, leftContext, rightPhone, rightContext, iii, jj, kk1;
	long curProb;
	unsigned short word1, word2;
	char basePitch, leftPitch, rightPitch;
	PHONENODE tokenPathTmp;
	long  lmProbTmp, lmProb[500], lookaheadProb[500];
	short modelID[500], modelIdTmp, localPathNum, kkk;//, modelIndex[3000], index;

	//the phone ID currently extended
	basePhone = m_lpLextree[baseGrammar].nodeId; //current phone extended
	basePitch = m_lpLextree[baseGrammar].toneId; //current pitch 
	ASSERT(basePhone > MAX_CODE_INITIAL_PART && basePhone != SILENCE_CODE);
	//
	leftContext = node.grammarNode; 
	leftPhone = m_lpLextree[leftContext].nodeId; //left context phone of current phone
	leftPitch = node.leftTone; //left context pitch 
	ASSERT(leftPhone <= MAX_CODE_INITIAL_PART);

	word1 = node.historyWord1;
	word2 = node.historyWord2;
	//insert new path into path stack
	tokenPathTmp.amScore = node.amScore;
	tokenPathTmp.grammarNode = baseGrammar;
	tokenPathTmp.historyWord1 = word1;
	tokenPathTmp.historyWord2 = word2;
	tokenPathTmp.preWordPos = node.preWordPos;
	tokenPathTmp.startTime = node.startTime;
	tokenPathTmp.tokenActive = 0;
	tokenPathTmp.wordDuration = 0;
	tokenPathTmp.leftTriphone = node.modelId;
	tokenPathTmp.leftTone = leftPitch; //从INITIAL->FINAL,传递左边的声调
	tokenPathTmp.nextToken = -1;
			
	//find all possible right contexts
	linkNo = m_lpLextree[baseGrammar].linkNum;
	stNode = m_lpLextree[baseGrammar].startNode;
	ASSERT(linkNo < 200);
	//
	//only right phone and right pitch is dynamical
	localPathNum = 0;
	//memset(modelIndex, -1, sizeof(short)*3000);
	for(iii = 0; iii < linkNo; iii++)
	{
		rightContext = stNode + iii;
		//dynamically calculate LM predictive probability
		curProb = m_LM->GetLMPredictScore(word1, word2, rightContext, m_lpLextree[rightContext].wordFirst,
		                              m_lpLextree[rightContext].wordLast, m_lpLextree[rightContext].maxUnigramProb, backWeight);

//		tokenPathTmp.lmLookaheadScore = curProb;
//		tokenPathTmp.lmScore = node.lmScore + (curProb - node.lmLookaheadScore);
		lmProbTmp = node.lmScore + (curProb - node.lmLookaheadScore);
		//
		if(ACOUSTIC_WEIGHT * node.amScore + LANGUAGE_WEIGHT * lmProbTmp >= g_pPath->m_lProbMax - maxOffsetLm)
		{
			if(m_lpLextree[rightContext].startNode > 0)
			{			
				//extend the first final and right context is INITIAL
				rightPhone = m_lpLextree[rightContext].nodeId;;		
				ASSERT(rightPhone <= MAX_CODE_INITIAL_PART);
				rightPitch = m_lpLextree[rightContext].toneId;
				//tokenPathTmp.modelId = m_AM->TriphoneMappingF(basePhone, basePitch, leftPitch, rightPitch, leftPhone, rightPhone);
				modelIdTmp = m_AM->TriphoneMappingF(basePhone, basePitch, leftPitch, rightPitch, leftPhone, rightPhone);
				
				//g_pPath->InsertPath(g_pPath->flag2, tokenPathTmp, 1);
				for(kkk = 0; kkk < localPathNum; kkk++)
				{
					if(modelID[kkk] == modelIdTmp)
					{
						if(lmProb[kkk] < lmProbTmp)
						{
							lmProb[kkk] = lmProbTmp;
							lookaheadProb[kkk] = curProb;
						}
						break;
					}
				}
				if(kkk == localPathNum)
				{
					lmProb[kkk] = lmProbTmp;
					lookaheadProb[kkk] = curProb;
					modelID[kkk] = modelIdTmp;
					localPathNum++;
					ASSERT(localPathNum <= 500);
				}
				/*
				index = modelIdTmp - m_AM->baseStartTricode[basePhone];
				if(modelIndex[index] < 0)
				{
					lmProb[localPathNum] = lmProbTmp;
					lookaheadProb[localPathNum] = curProb;
					modelID[localPathNum] = modelIdTmp;
					modelIndex[index] = localPathNum;
					localPathNum++;
					ASSERT(localPathNum <= 500);
				}
				else
				{
					index = modelIndex[index];
					if(lmProb[index] < lmProbTmp)
					{
						lmProb[index] = lmProbTmp;
						lookaheadProb[index] = curProb;
					}
				}*/
			}
			else
			{	
				ASSERT(m_lpLextree[rightContext].nodeId == SILENCE_CODE);
				rightPhone = m_lpLextree[rightContext].nodeId;
				ASSERT(rightPhone == SILENCE_CODE);
				for(jj = 0; jj <= PITCH_NULL; jj++)
				{
					//tokenPathTmp.modelId = m_AM->TriphoneMappingF(basePhone, basePitch, leftPitch, jj, leftPhone, rightPhone);
					modelIdTmp = m_AM->TriphoneMappingF(basePhone, basePitch, leftPitch, jj, leftPhone, rightPhone);
					//g_pPath->InsertPath(g_pPath->flag2, tokenPathTmp, 1);
					for(kkk = 0; kkk < localPathNum; kkk++)
					{
						if(modelID[kkk] == modelIdTmp)
						{
							if(lmProb[kkk] < lmProbTmp)
							{
								lmProb[kkk] = lmProbTmp;
								lookaheadProb[kkk] = curProb;
							}
							break;
						}
					}
					if(kkk == localPathNum)
					{
						lmProb[kkk] = lmProbTmp;
						lookaheadProb[kkk] = curProb;
						modelID[kkk] = modelIdTmp;
						localPathNum++;
						ASSERT(localPathNum <= 500);
					}
					/*
					index = modelIdTmp - m_AM->baseStartTricode[basePhone];
					if(modelIndex[index] < 0)
					{
						lmProb[localPathNum] = lmProbTmp;
						lookaheadProb[localPathNum] = curProb;
						modelID[localPathNum] = modelIdTmp;
						modelIndex[index] = localPathNum;
						localPathNum++;
						ASSERT(localPathNum <= 500);
					}
					else
					{	
						index = modelIndex[index];
						if(lmProb[index] < lmProbTmp)
						{
							lmProb[index] = lmProbTmp;
							lookaheadProb[index] = curProb;
						}
					}*/
				}
				//right context is the first INITIAL of the next word
				linkNoTmp = m_lpLextree[0].linkNum;
				stNodeTmp = m_lpLextree[0].startNode;
			
				for(kk1 = 0; kk1 < linkNoTmp; kk1++)
				{
					rightContext = stNodeTmp + kk1;
					rightPhone = m_lpLextree[rightContext].nodeId;
					rightPitch = m_lpLextree[rightContext].toneId;
					ASSERT(rightPhone <= MAX_CODE_INITIAL_PART);
					//tokenPathTmp.modelId = m_AM->TriphoneMappingF(basePhone, basePitch, leftPitch, rightPitch, leftPhone, rightPhone);
					modelIdTmp = m_AM->TriphoneMappingF(basePhone, basePitch, leftPitch, rightPitch, leftPhone, rightPhone);
					//insert new path into path stack
					//g_pPath->InsertPath(g_pPath->flag2, tokenPathTmp, 1);
					for(kkk = 0; kkk < localPathNum; kkk++)
					{
						if(modelID[kkk] == modelIdTmp)
						{
							if(lmProb[kkk] < lmProbTmp)
							{
								lmProb[kkk] = lmProbTmp;
								lookaheadProb[kkk] = curProb;
							}
							break;
						}
					}
					if(kkk == localPathNum)
					{
						lmProb[kkk] = lmProbTmp;
						lookaheadProb[kkk] = curProb;
						modelID[kkk] = modelIdTmp;
						localPathNum++;
						ASSERT(localPathNum <= 500);
					}
				/*
					index = modelIdTmp - m_AM->baseStartTricode[basePhone];
					if(modelIndex[index] < 0)
					{
						lmProb[localPathNum] = lmProbTmp;
						lookaheadProb[localPathNum] = curProb;
						modelID[localPathNum] = modelIdTmp;
						modelIndex[index] = localPathNum;
						localPathNum++;
						ASSERT(localPathNum <= 500);
					}
					else
					{	
						index = modelIndex[index];
						if(lmProb[index] < lmProbTmp)
						{
							lmProb[index] = lmProbTmp;
							lookaheadProb[index] = curProb;
						}
					}*/
				}
			}
		}
	}
	//
	for(kkk = 0; kkk < localPathNum; kkk++)
	{
		tokenPathTmp.lmLookaheadScore = lookaheadProb[kkk];
		tokenPathTmp.lmScore = lmProb[kkk];
		tokenPathTmp.modelId = modelID[kkk];
		//Nov.3,2000
#ifdef USE_AM_PREDICT
		long deltaAm = 0;
		if(g_pAM->m_nextCodeWord >= 0)
			deltaAm = g_pAM->GetObserveProb(tokenPathTmp.modelId, 0, 1, g_pAM->m_nextCodeWord);
		//
		if(ACOUSTIC_WEIGHT * (tokenPathTmp.amScore + deltaAm) + LANGUAGE_WEIGHT * tokenPathTmp.lmScore >= g_pPath->m_lProbMax - maxOffsetLm + 70000) 
#endif
		{
//			tokenPathTmp.deltaScore = deltaAm;
			g_pPath->InsertPath(g_pPath->flag2, tokenPathTmp, 1);
		}
	}
}


void CLextree::FlyingExtendInitial(PHONENODE& node, int baseGrammar, long backWeight)
{
	int linkNo, stNode, basePhone, leftPhone, leftContext, rightPhone, rightContext, ii;
	long curProb;
	unsigned short word1, word2;
	PHONENODE tokenPathTmp;

	leftContext = node.grammarNode;
	//leftPhone = mappingPitch[m_lpLextree[leftContext].nodeId];
	leftPhone = m_lpLextree[leftContext].nodeId;
	ASSERT(leftPhone > MAX_CODE_INITIAL_PART);
	//
	word1 = node.historyWord1;
	word2 = node.historyWord2;
	//insert new path into path stack			
	tokenPathTmp.amScore = node.amScore;
	tokenPathTmp.historyWord1 = word1;
	tokenPathTmp.historyWord2 = word2;
	tokenPathTmp.preWordPos = node.preWordPos;
	tokenPathTmp.startTime = node.startTime;
	tokenPathTmp.tokenActive = 0;
	tokenPathTmp.wordDuration = 0;
	tokenPathTmp.leftTriphone = node.modelId;
	tokenPathTmp.leftTone = m_lpLextree[leftContext].toneId; //SILENCE->INITIAL/FINAL-->INITIAL,更新声调,记录当前模型左边的声调			
	tokenPathTmp.nextToken = -1;
	tokenPathTmp.grammarNode = baseGrammar;

	//find all possible right contexts
	linkNo = m_lpLextree[baseGrammar].linkNum;
	stNode = m_lpLextree[baseGrammar].startNode;
	ASSERT(linkNo < 200);
	//basePhone = mappingPitch[m_lpLextree[baseGrammar].nodeId];
	basePhone = m_lpLextree[baseGrammar].nodeId;
	ASSERT(basePhone <= MAX_CODE_INITIAL_PART);
	//
	for(ii = 0; ii < linkNo; ii++)
	{
		rightContext = stNode + ii;
		//hmmModelCode = m_lpLextree[rightContext].nodeId;
		//rightPhone = mappingPitch[hmmModelCode];
		rightPhone = m_lpLextree[rightContext].nodeId;
		ASSERT(rightPhone > MAX_CODE_INITIAL_PART && rightPhone != SILENCE_CODE);
		//
		curProb = m_LM->GetLMPredictScore(word1, word2, rightContext, m_lpLextree[rightContext].wordFirst,
		                              m_lpLextree[rightContext].wordLast, m_lpLextree[rightContext].maxUnigramProb, backWeight);

		//
		tokenPathTmp.modelId = m_AM->TriphoneMappingI(basePhone, leftPhone, rightPhone);
		tokenPathTmp.lmScore = node.lmScore + (curProb - node.lmLookaheadScore);
		tokenPathTmp.lmLookaheadScore = curProb;
		//
		//Nov.3,2000
		long sumScore = ACOUSTIC_WEIGHT * tokenPathTmp.amScore + LANGUAGE_WEIGHT * tokenPathTmp.lmScore;
		//
		if(sumScore >= g_pPath->m_lProbMax - maxOffsetLm)
		{
#ifdef USE_AM_PREDICT
			long deltaAm = 0;
			if(g_pAM->m_nextCodeWord >= 0)
				deltaAm = g_pAM->GetObserveProb(tokenPathTmp.modelId, 0, 1, g_pAM->m_nextCodeWord);
			if(sumScore + ACOUSTIC_WEIGHT * deltaAm >= g_pPath->m_lProbMax - maxOffsetLm + 70000)
#endif
			{
//				tokenPathTmp.deltaScore = deltaAm;
				g_pPath->InsertPath(g_pPath->flag2, tokenPathTmp, 1);
			}
		}
	}
}

void CLextree::CollectWord(PHONENODE &node,int nEndNode,int nTime)
{
	
	int nLeastIdx, nLastNode, nLmProbIdx;
	short nLastTrihone,nPrePos,nPreTime;
	long lMinProb,lScoreAM, lScoreLM, LMScore[200], sumScore;
	WORD wXWord, wYWord, wCurWord, wWord1, wWord2,nWordNum;
	char cLeftTone, cBaseTone;

	ASSERT(m_lpLextree[nEndNode].startNode < 0);

	//从LeftContext的继承相关信息
	nLastNode = node.grammarNode;
	nLastTrihone = node.modelId;
	wWord1 = node.historyWord1;
	wWord2 = node.historyWord2;
	cLeftTone = node.leftTone;
	cBaseTone = m_lpLextree[nLastNode].toneId;
	nPreTime = (short)(node.startTime - 1);
	nPrePos = node.preWordPos; 
	lScoreAM = node.amScore;
	lScoreLM = node.lmScore;

	//得到候选词的词号范围和总数
	wXWord = m_lpLextree[nEndNode].wordFirst;
	wYWord = m_lpLextree[nEndNode].wordLast;
	nWordNum = (WORD)(wYWord - wXWord + 1);
	ASSERT(nWordNum < 200);

	//计算出所有候选词的LM得分
	long lPredictProb = node.lmLookaheadScore;
	long lProbBackoff = m_LM->CheckTriBackoff(wWord1, wWord2);

	for(WORD i = 0; i < nWordNum; i++)
	{
		wCurWord = (WORD)(wXWord + i);
		LMScore[i] = m_LM->GetLMScore(wWord1, wWord2, wCurWord, lProbBackoff)
					+ lScoreLM - lPredictProb;
	}

	//选出候选词
	for(WORD wordCode = wXWord; wordCode <= wYWord; wordCode++)
	{
		int counter = m_nWordNumber;
		nLmProbIdx = wordCode - wXWord;
//
		sumScore = ACOUSTIC_WEIGHT * lScoreAM + LANGUAGE_WEIGHT_WORD * LMScore[nLmProbIdx];

		if(sumScore >= g_pPath->m_lProbMax - maxOffsetLm)
		{
			//检测当前词(要区分LastTriphone)是否已存在？是,只保留最一个好的
			for(i = 0; i < counter; i++)
			{
				if((m_WordList[i].wordId == wordCode)
					&&(m_WordList[i].hisWord2 == wWord2)
					&&(m_WordList[i].lastModel == nLastTrihone))
				{
					//比较概率得分,保留大的那个
					if(ACOUSTIC_WEIGHT * lScoreAM + 
						LANGUAGE_WEIGHT_WORD * LMScore[nLmProbIdx] 
						> ACOUSTIC_WEIGHT * m_WordList[i].amScore 
						+ LANGUAGE_WEIGHT_WORD * m_WordList[i].lmScore)
					{
						m_WordList[i].amScore = lScoreAM;
						m_WordList[i].lmScore = LMScore[nLmProbIdx];
						m_WordList[i].leftTriphone = node.leftTriphone;
						m_WordList[i].preWordPos = node.preWordPos;
						m_WordList[i].preTime = nPreTime;
						m_WordList[i].hisWord1 = wWord1;
						m_WordList[i].toneId = cBaseTone;
						m_WordList[i].leftTone = cLeftTone;
						m_WordList[i].modelDur = node.wordDuration;
					}
					break;
				}
			}
			if(i < counter)
				continue;

			//处理当前词尚未别列入候选的情况
			if(counter < MAX_WORD_CANDIDATES)
			//候选表未满,则将其列入候选
			{
				m_WordList[counter].wordId = wordCode;
				m_WordList[counter].lastModel = nLastTrihone; //curPhone;
				m_WordList[counter].preTime = nPreTime;
				m_WordList[counter].preWordPos = nPrePos;
				m_WordList[counter].amScore = lScoreAM;
				m_WordList[counter].lmScore = LMScore[nLmProbIdx];
				m_WordList[counter].hisWord1 = wWord1;
				m_WordList[counter].hisWord2 = wWord2;
				m_WordList[counter].leftTriphone = node.leftTriphone;
				m_WordList[counter].toneId = cBaseTone;
				m_WordList[counter].leftTone = cLeftTone;
				m_WordList[counter].modelDur = node.wordDuration;

				m_nWordNumber++;
			}
			else
			//候选表已满,与得分最小的那个比较,保留这两者中得分高的
			{
				lMinProb = POS_HUGE;
				nLeastIdx = -1;
				//找出得分最小的那个候选词
				for(i = 0; i < counter; i++)
				{
					long probTmp = ACOUSTIC_WEIGHT * m_WordList[i].amScore +
						LANGUAGE_WEIGHT_WORD * m_WordList[i].lmScore;
					if(lMinProb > probTmp)
					{
						lMinProb = probTmp;
						nLeastIdx = i;
					}
				}
				ASSERT(nLeastIdx >= 0);
				//比较两者得分,保留得分高的
				if(lMinProb < ACOUSTIC_WEIGHT * lScoreAM + LANGUAGE_WEIGHT_WORD * LMScore[nLmProbIdx])
				{
					m_WordList[nLeastIdx].wordId = wordCode;
					m_WordList[nLeastIdx].lastModel = nLastTrihone; //curPhone;
					m_WordList[nLeastIdx].preTime = nPreTime;
					m_WordList[nLeastIdx].preWordPos = nPrePos;
					m_WordList[nLeastIdx].amScore = lScoreAM;
					m_WordList[nLeastIdx].lmScore = LMScore[nLmProbIdx];
					m_WordList[nLeastIdx].hisWord1 = wWord1;
					m_WordList[nLeastIdx].hisWord2 = wWord2;
					m_WordList[nLeastIdx].leftTriphone = node.leftTriphone;
					m_WordList[nLeastIdx].toneId = cBaseTone;
					m_WordList[nLeastIdx].leftTone = cLeftTone;
					m_WordList[nLeastIdx].modelDur = node.wordDuration;
				}
			}
		}
	}
}

void CLextree::LoadLextree(short loadMode)
{
	char sFilePath[MAX_PATH];
	HANDLE hLextree;
	
	char lexTopic[50];
	switch(loadMode)
	{
		case DICTATION_MODE:
			strcpy(lexTopic, "Dictation Mode");
			break;
		case COMMAND_MODE:
			strcpy(lexTopic, "Command Mode");
			break;
		case ADAPT_MODE:
			strcpy(lexTopic, "Adapt Mode");
			break;
		default:
			break;
	}
	//取得lextree文件的全路径
	DWORD dwSize = GetPrivateProfileString(
		"Lextree",
		lexTopic,
		NULL,
		sFilePath,
		MAX_PATH,
		"SearchEngine.ini");

	ASSERT(dwSize != 0);

	//打开lextree文件
	hLextree = CreateFile(
		sFilePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
		NULL);
	if (hLextree == INVALID_HANDLE_VALUE)
	{
		TRACE1("Open Lextree File Failed!\n",GetLastError());
		return;
	}
	//得到lextree文件大小
	dwSize = GetFileSize(hLextree,NULL);
	
	ASSERT(dwSize != 0xFFFFFFFF);

	//得到扇区的size
	DWORD	dwSectorsPerCluster, 
			dwBytesPerSector,  
			dwNumberOfFreeClusters,
   			dwTotalNumberOfClusters = 0;
	
	BOOL bReturn = GetDiskFreeSpace(
			NULL,
			&dwSectorsPerCluster, 
			&dwBytesPerSector,  
			&dwNumberOfFreeClusters,
   			&dwTotalNumberOfClusters);
	if (!bReturn)
	{
		TRACE1("Get Size of PerSector of Disk Error:%d\n",GetLastError());
		return;
	}
	
	//计算所需扇区数
	DWORD dwNeedSectorNum = ((dwSize%dwBytesPerSector)!=0) 
					? (dwSize/dwBytesPerSector+1) : (dwSize/dwBytesPerSector);

	//分配m_lpLextre内存
	m_lpLextreeMode[loadMode] = (LPLEXTREE)VirtualAlloc(
			NULL,
			dwSize,
			//dwBytesPerSector*dwNeedSectorNum,
			MEM_COMMIT,
			PAGE_READWRITE);

	//读取lextree文件
	DWORD	dwNumBytesRead = 0; 
	if (!ReadFile(
		hLextree,
		(LPVOID)m_lpLextreeMode[loadMode],
		dwBytesPerSector*dwNeedSectorNum,
		&dwNumBytesRead,
		NULL))
	{
		TRACE1("Read Lextree File Error:%d\n",GetLastError());
		VirtualFree(m_lpLextreeMode[loadMode],0,MEM_RELEASE);
		CloseHandle(hLextree);
		return;
	}

	ASSERT(dwSize == dwNumBytesRead);
	CloseHandle(hLextree);
}

void CLextree::UnLoadLextree(short loadMode)
{
	if (m_lpLextreeMode[loadMode])
		VirtualFree(m_lpLextreeMode[loadMode],0,MEM_RELEASE);
}
void CLextree::TreeLeafExtend(PHONENODE &node, int newGrammar, long curProb)
{
	int leftContext, leftPhone;
	PHONENODE tokenPathTmp;
	char leftPitch;

	leftContext = node.grammarNode;
	leftPhone = m_lpLextree[leftContext].nodeId;
	leftPitch = m_lpLextree[leftContext].toneId;
	ASSERT(leftPhone != SILENCE_CODE && leftPhone > MAX_CODE_INITIAL_PART);
	ASSERT(m_lpLextree[newGrammar].nodeId == SILENCE_CODE);
	//insert new path into path stack
	tokenPathTmp.amScore = node.amScore;
	tokenPathTmp.lmScore = node.lmScore + (curProb - node.lmLookaheadScore);
	tokenPathTmp.grammarNode = newGrammar;
	tokenPathTmp.historyWord1 = node.historyWord1;
	tokenPathTmp.historyWord2 = node.historyWord2;
	tokenPathTmp.lmLookaheadScore = curProb;
	tokenPathTmp.modelId = g_pAM->m_triSilenceCode;//TRI_SILENCE_CODE;
	tokenPathTmp.preWordPos = node.preWordPos;
	tokenPathTmp.startTime = node.startTime;
	tokenPathTmp.tokenActive = 0;
	tokenPathTmp.wordDuration = 0;
	tokenPathTmp.leftTriphone = node.modelId;
	tokenPathTmp.nextToken = -1;
	tokenPathTmp.leftTone = node.leftTone; //从FINAL->SILENCE,传递前一音节的声调

	//Nov.3,2000
	//
	long sumScore = ACOUSTIC_WEIGHT * tokenPathTmp.amScore + LANGUAGE_WEIGHT * tokenPathTmp.lmScore;
	if(sumScore >= g_pPath->m_lProbMax - maxOffsetLm)
	{
#ifdef USE_AM_PREDICT
		long deltaAm = 0;
		if(g_pAM->m_nextCodeWord >= 0)
			deltaAm = g_pAM->GetObserveProb(tokenPathTmp.modelId, 0, 1, g_pAM->m_nextCodeWord);
		if(sumScore + ACOUSTIC_WEIGHT * deltaAm >= g_pPath->m_lProbMax - maxOffsetLm + 70000)
#endif
		{
//			tokenPathTmp.deltaScore = deltaAm;
			g_pPath->InsertPath(g_pPath->flag2, tokenPathTmp, 1);	
		}
	}
}
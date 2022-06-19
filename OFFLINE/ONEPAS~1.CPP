/**************************************************************************************************************
文件名：onepassSearch.cpp
说明：一遍搜索的主框架
作者：高升
日期：Sep.27,2000
**************************************************************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>
#include	<assert.h>
#include	<float.h>
#include	<string.h>
#include	<conio.h>

#include	"include\const.h"
#include	"include\struct.h"
#include	"include\global.h"

//double expmin=pos_huge, expmax=neg_huge, logmin=pos_huge,logmax=neg_huge;
//int gaunum=0;
//int gaushort=0;

//#define SORT_LATTICE
#define	TRIGRAM_PREDICT
#define	LM_CACHE
#define	FAST_COMPUTE
#define	USE_PITCH

//函数声明
long	GetObserveProb(short nModel, short nState1, short nState2, float *pVector);
long	GetObserveProbFast(short nModel, short nState1, short nState2, float *pVector, short codeWord);
long	GetTransProb(short nModel, short nState1, short nState2);

long	GetCacheProb(unsigned short w1, unsigned short w2, int node, long triBackWeight);
long	GetDisLmProb(unsigned short, unsigned short wFirstID, int childNode, long);
long	GetDisTrigram(unsigned short w1, unsigned short w2, unsigned short xword, unsigned short yword);
long	GetDisBigram(unsigned short w1, unsigned short xWord, unsigned short yWord);
long	CheckTrigram(unsigned short w1, unsigned short w2, unsigned short w3);
long	CheckTriBackoff(unsigned short w1, unsigned short w2);
long	CheckBigram(unsigned short w1, unsigned short w2);

int		InsertPath(int stackIdx, CPhoneNode path, int);
void	ReleaseBlock(int flag1, int keyCt, int, int curIndex);
short	FindCodebook(float*);

void	InitPath(void);
void	IntraHmmExtend(int);
void	InterHmmExtend(int);
void	CrossWordExtend(int tt);
void	TreeLeafExtend(int pos, int newGrammar, long);
void	FlyingExtendFinal(int pos, int baseGrammar, long);
void	FlyingExtendInitial(int pos, int, long);
void	CollectWord(int time, int stackIdx, int pos, int);
void	BeamPruning(int startNo, int addNum, int threshold, const long*);

void	TraceBack(void);
void	SortLattice(int time);
inline int	triphoneMapInitial(short base, short, short, short, short left, short right);
inline int	triphoneMapFinal(short base, short, short, short, short left, short right);

void	ReleaseMemory(void);

//主搜索框架
void	OnepassSearchEngine()
{
	//变量声明
	int tt, temp, jj;
	//
	for(tt = 0; tt < speechDataLength; tt++)
		wordNumber[tt] = 0;
	for(jj = 0; jj < MODULE_VALUE1+MODULE_VALUE2; jj++)
		pBucketHead[0][jj] = pBucketHead[1][jj] = -1;
	for(jj = 0; jj < NSLmax; jj++)
		freeBlockIdx[jj] = jj;
	freeBlockTop = 0;

	//路径初始化
	InitPath();
	//搜索 
	maxProbThreshold = neg_huge;
	//
	for(tt = 0; tt < speechDataLength; tt++)
	{
		//HMM内状态间扩展和裁剪
		IntraHmmExtend(tt);
		BeamPruning(0, MODULE_VALUE1, INTRA_WORD_THRESHOLD, M_EPS_INTRA);
		//更新模型间裁剪的出门限
		maxProbThreshold -= 300000;
		//模型间扩展和收集词
		InterHmmExtend(tt);
#ifdef SORT_LATTICE
		SortLattice(tt);
#endif
		//词间扩展
		CrossWordExtend(tt);
		//模型间路径裁剪
		BeamPruning(MODULE_VALUE1, MODULE_VALUE1 + MODULE_VALUE2, INTER_WORD_THRESHOLD, M_EPS_INTER);
		//更新模型内扩展的初始门限
		//maxProbThreshold = maxProbThreshold + 70000;//10000;//leastActiveProb;// - 8000;
		maxProbThreshold = maxProbThreshold - 300000;
		//交换路径堆栈
		temp = flag1;
		flag1 = flag2;
		flag2 = temp;
		//
		for(jj = 0; jj < MODULE_VALUE1+MODULE_VALUE2; jj++)
			pBucketHead[flag2][jj] = -1;
	}
	//
	TraceBack();
	//
	//printf("%d %d\n", gaunum, gaushort);
}

// Probability Calculation Interface
long GetTransProb(short nModel, short nState1, short nState2)
{
	int nTrans;
	nTrans=Hmm[nModel].aLinks[nState1*Hmm[nModel].No_State+nState2];
	//long x =  (long)(log10(Hmm[nModel].A[nTrans]) / logbase);
	//long x =  (long)(log10(Hmm[nModel].A[nTrans]) * logbase);
	long x =  Hmm[nModel].A[nTrans];
	return x;
}

/*
long GetObserveProb(short int nModel, short int iState, short int jState, float *pTmp)
{
	double zhishu,xishu,fenzi,fenmu;
	double TransLikelihood;
	int ii,nOut, j;
	long temp;

	TransLikelihood = 0;

	nOut = Hmm[nModel].bLinks[iState*Hmm[nModel].No_State+jState];
	nOut = Hmm[nModel].OutputID[nOut];
	
	assert(nOut < No_Output);
	if(ProbBuffer[nOut] != neg_huge)
		return ProbBuffer[nOut];		

	for(ii = 0; ii < No_Mix; ii++)
	{
		zhishu = 0;
		xishu = pai2;

		for(j = 0; j < No_Dim; j++)
		{
#ifndef USE_PITCH			
			if((j + 1) % 14 == 0)
				continue;
#endif			
			temp = ii * No_Dim + j;
			fenzi=pTmp[j] - Output[nOut].Means[temp];
			fenzi *= fenzi;

			fenmu=Output[nOut].Variance[temp];
			zhishu += fenzi/fenmu;
			xishu *= fenmu;
		}
	
		fenzi = exp(-0.5 * zhishu);
		fenmu = sqrt(xishu);

		TransLikelihood += Output[nOut].CoeMix[ii] * (fenzi / fenmu);
	}
	long x;
	if(TransLikelihood != 0.0)
		x = (long)(log10(TransLikelihood) / logbase);
	else
		x = neg_huge-1;

	ProbBuffer[nOut] = x;
	return x;  
}
*/

long GetObserveProb(short int nModel, short int iState, short int jState, float *pTmp)
{
	double zhishu, fenzi;
	double TransLikelihood;
	int ii,nOut, j;
	long temp;
	float *pm, *pv, *pc;

	TransLikelihood = 0;

	nOut = Hmm[nModel].bLinks[iState*Hmm[nModel].No_State+jState];
	nOut = Hmm[nModel].OutputID[nOut];
	
	assert(nOut < No_Output);
	if(ProbBuffer[nOut] != neg_huge)
		return ProbBuffer[nOut];		

	pm = Output[nOut].Means;
	pv = Output[nOut].Variance;
	pc = Output[nOut].CoeMix;

	for(ii = 0; ii < No_Mix; ii++)
	{
		zhishu = 0;
		//xishu = 1;//pai2;

		for(j = 0; j < No_Dim; j++)
		{
#ifndef USE_PITCH			
			if((j + 1) % 14 == 0)
				continue;
#endif			
			temp = ii * No_Dim + j;
			fenzi=pTmp[j] - pm[temp];//Output[nOut].Means[temp];
			fenzi *= fenzi;
			//fenmu=Output[nOut].Variance[temp];
			zhishu += fenzi * pv[temp];//Output[nOut].Variance[temp];
		//	xishu *= fenmu;
		}
	
		fenzi = exp(-zhishu);
		//fenmu = sqrt(xishu);

		//TransLikelihood += Output[nOut].CoeMix[ii] * fenzi;// * fenmu;
		TransLikelihood += pc[ii] * fenzi;
	}
	long x;

	if(TransLikelihood != 0.0)
		//x = (long)(log10(TransLikelihood) / logbase);
		x = (long)(log10(TransLikelihood) * logbase);
	else
		x = neg_huge-1;

	ProbBuffer[nOut] = x;
	return x;  
}

//采用short list 快速计算
long GetObserveProbFast(short int nModel, short int iState, short int jState, float *pTmp, short vqCode)
{
	double zhishu, fenzi;
	double TransLikelihood;
	int ii,nOut, j;
	long temp, x;
	float *pm, *pv, *pc;
	short index;
	
/*///////////
	
	TransLikelihood = 0;

	nOut = Hmm[nModel].bLinks[iState*Hmm[nModel].No_State+jState];
	nOut = Hmm[nModel].OutputID[nOut];
	
	assert(nOut < No_Output);
	if(ProbBuffer[nOut] != neg_huge)
		return ProbBuffer[nOut];		

	pm = Output[nOut].Means;
	pv = Output[nOut].Variance;
	pc = Output[nOut].CoeMix;

	for(ii = 0; ii < No_Mix; ii++)
	{
		zhishu = 0;
		//xishu = 1;//pai2;

		for(j = 0; j < No_Dim; j++)
		{
#ifndef USE_PITCH			
			if((j + 1) % 14 == 0)
				continue;
#endif			
			temp = ii * No_Dim + j;
			fenzi=pTmp[j] - pm[temp];//Output[nOut].Means[temp];
			fenzi *= fenzi;
			//fenmu=Output[nOut].Variance[temp];
			zhishu += fenzi * pv[temp];//Output[nOut].Variance[temp];
		//	xishu *= fenmu;
		}
	
		fenzi = exp(-zhishu);
		//fenmu = sqrt(xishu);

		//TransLikelihood += Output[nOut].CoeMix[ii] * fenzi;// * fenmu;
		TransLikelihood += pc[ii] * fenzi;
	}

	if(TransLikelihood != 0.0)
		//x = (long)(log10(TransLikelihood) / logbase);
		x = (long)(log10(TransLikelihood) * logbase);
	else
		x = neg_huge-1;
	long x1=x;	
	
//////////////*/	
	TransLikelihood = 0.0;

	nOut = Hmm[nModel].bLinks[iState*Hmm[nModel].No_State+jState];
	nOut = Hmm[nModel].OutputID[nOut];
	
	assert(nOut < No_Output);
	if(ProbBuffer[nOut] != neg_huge)
		return ProbBuffer[nOut];

	int listNum, offset;

	pm = Output[nOut].Means;
	pv = Output[nOut].Variance;
	pc = Output[nOut].CoeMix;

	listNum = shortListNum[vqCode*No_Output+nOut];
	offset = shortListOffset[vqCode*No_Output+nOut];
	//
//gaunum+=No_Mix;
//gaushort+=listNum;

	for(ii = 0; ii < listNum; ii++)
	{
		zhishu = 0;
		//xishu = 1;//pai2;
		index = shortList[offset+ii];
		for(j = 0; j < No_Dim; j++)
		{
#ifndef USE_PITCH			
			if((j + 1) % 14 == 0)
				continue;
#endif			
			temp = index * No_Dim + j;
			fenzi=pTmp[j] - pm[temp];//Output[nOut].Means[temp];
			fenzi *= fenzi;
			//fenmu=Output[nOut].Variance[temp];
			zhishu += fenzi * pv[temp];//Output[nOut].Variance[temp];
		//	xishu *= fenmu;
		}

//if(-zhishu<expmin)
  //expmin=-zhishu;
//if(-zhishu>expmax)
//	expmax=-zhishu;
#ifdef CHECK_EXP_TABLE
		int expIdx = zhishu * EXP_STEP;
		if(expIdx >= EXP_TABLE_SIZE)
			expIdx = EXP_TABLE_SIZE - 1;
		fenzi = expTable[expIdx];
#else
		fenzi = exp(-zhishu);
#endif
		//fenmu = sqrt(xishu);

		//TransLikelihood += Output[nOut].CoeMix[ii] * fenzi;// * fenmu;
		TransLikelihood += pc[index] * fenzi;
	}

//if(TransLikelihood<logmin&&TransLikelihood>0.0)
//logmin=TransLikelihood;
//if(TransLikelihood>logmax)
//logmax=TransLikelihood;

	if(TransLikelihood != 0.0)
		//x = (long)(log10(TransLikelihood) / logbase);
		x = (long)(log10(TransLikelihood) * logbase);
	else
		x = neg_huge-1;

	ProbBuffer[nOut] = x;
	return x;  
}

void ReleaseMemory()
{
	delete[] pHanzi;
	delete[] pBigramIdx;
	delete[] pBigram;
	delete[] pLexicalTree;
	delete[] pTrigramIdx;
	delete[] pTrigram;
	delete[] pTriphoneMapInitial;
	delete[] pTriphoneMapFinal;

	for(int i = 0; i < TRI_NO_HMM; i++)
	{
		delete[] Hmm[i].A;
		delete[] Hmm[i].aLinks;
		delete[] Hmm[i].bLinks;
		delete[] Hmm[i].indj;
		delete[] Hmm[i].nj;
		delete[] Hmm[i].OutputID;
	}
	for(i = 0; i < No_Output; i++)
	{	
		delete []Output[i].CoeMix;
		delete []Output[i].Means;
		delete []Output[i].Variance;
	}
	//
	delete	[]shortListNum;
	delete	[]shortListOffset;
	delete	[]shortList;
	//delete	totalMean;
	//delete	totalVar;
	delete	[]clusterMean;
	delete	[]clusterVar;
	//
	delete	[]pLmCodebook;
#ifdef	CHECK_EXP_TABLE
	delete	[]expTable;
#endif
}

void BeamPruning(int startNo, int moduleValue, int pathThreshold, const long *M_EPS)
{
	// Do Intra-phone Transition Pruning
	long probMax, probTmp;
	int iii, curIndex, preIndex, keyCt;
	short	codeNum[BLOCKNUM+1]; 

/*
	probMax = neg_huge;
	for(keyCt = startNo; keyCt < moduleValue; keyCt++)
	{
		curIndex = pBucketHead[flag2][keyCt];
		while(curIndex >= 0)
		{
			probTmp = ACOUSTIC_WEIGHT * tokenPath[curIndex].amScore + LANGUAGE_WEIGHT * tokenPath[curIndex].lmScore;
			if(probTmp > probMax)
				probMax = probTmp;
			curIndex = tokenPath[curIndex].nextToken;
		}
	}
*/
	for(iii = 0; iii <= BLOCKNUM; iii++)
		codeNum[iii] = 0;

	probMax = maxProbThreshold;
	for(keyCt = startNo; keyCt < moduleValue; keyCt++)
	{
		curIndex = pBucketHead[flag2][keyCt];
		preIndex = -1;
		while(curIndex >= 0)
		{
			probTmp = ACOUSTIC_WEIGHT * tokenPath[curIndex].amScore + LANGUAGE_WEIGHT * tokenPath[curIndex].lmScore;
			for(iii = 0; iii < BLOCKNUM; iii++)
			{
				if(probTmp >= (probMax - M_EPS[iii]))
				{
					tokenPath[curIndex].tokenPruneLevel = iii;
					codeNum[iii]++;
					break;
				}
			}
			if(iii == BLOCKNUM)
			{
				tokenPath[curIndex].tokenPruneLevel = BLOCKNUM;
				codeNum[BLOCKNUM]++;
			}
			preIndex = curIndex;
			curIndex = tokenPath[curIndex].nextToken;
		}
	 }

	int totalNum = 0;    
	int code = BLOCKNUM - 1;
	for(iii = 0; iii < BLOCKNUM; iii++)
	{   
		totalNum += codeNum[iii];
		if(totalNum >= pathThreshold)
		{
			code = iii; 
			break;
		}
	}

	totalNum=0;
	for(keyCt = startNo; keyCt < moduleValue; keyCt++)
	{
		curIndex = pBucketHead[flag2][keyCt];
		preIndex = -1;
		while(curIndex >= 0)
		{
			//
			//pathNum++;
			//
			if(tokenPath[curIndex].tokenPruneLevel <= code)
			{
				//set this path activated
				totalNum++;
				preIndex = curIndex;
			}
			else
			{
				//
				//prunePathNum++;
				//
				//release this block
				ReleaseBlock(flag2, keyCt, preIndex, curIndex);
				preIndex = preIndex;
			}
			curIndex = tokenPath[curIndex].nextToken;
		}
	}
}


/***************************************************************************/
//extended FINALs have two types:
//1. final is the first final of word but not last. In this case,the word has
//   more than 2 characthers(syllables)
//2. final is the first and last final of word. In this case,the word has only 
//   1 character(syllable) and treat it as the last final.
//3. final is the last final of word but the first. In this case,the word has
//   more than 2 characthers(syllables)
/**************************************************************************/
/*
void	FlyingExtendFinal(int pos, int baseGrammar, long backWeight)
{
	int linkNo, stNode, linkNoTmp, stNodeTmp, basePhone, leftPhone, leftContext, rightPhone, rightContext, iii, hmmModelCode, jj,
		nextLinkNo, nextStNode, node, kk1;
	long probAm, probLm, probFather, curProb;
	unsigned short word1, word2;
	CPhoneNode tokenPathTmp;
	char basePitch, leftPitch, rightPitch;

	//the phone ID currently extended
	basePhone = mapping[pLexicalTree[baseGrammar].nodeId]; //current phone extended
	assert(pLexicalTree[baseGrammar].nodeId >= TRI_SILENCE_CODE);
	basePitch = pLexicalTree[baseGrammar].toneId; //current pitch 
	assert(basePhone > MAX_CODE_INITIAL_PART && basePhone != SILENCE_CODE);
	//
	probFather = tokenPath[pos].lmLookaheadScore;
	leftContext = tokenPath[pos].grammarNode; 
	leftPhone = mapping[pLexicalTree[leftContext].nodeId]; //left context phone of current phone
	leftPitch = tokenPath[pos].leftTone; //left context pitch 
	assert(leftPhone <= MAX_CODE_INITIAL_PART);

	word1 = tokenPath[pos].historyWord1;
	word2 = tokenPath[pos].historyWord2;

	probAm = tokenPath[pos].amScore;
	probLm = tokenPath[pos].lmScore;

	//insert new path into path stack
	tokenPathTmp.amScore = probAm;
	tokenPathTmp.grammarNode = baseGrammar;
	tokenPathTmp.historyWord1 = word1;
	tokenPathTmp.historyWord2 = word2;
	tokenPathTmp.preWordPos = tokenPath[pos].preWordPos;
	tokenPathTmp.startTime = tokenPath[pos].startTime;
	tokenPathTmp.tokenActive = 0;
	tokenPathTmp.wordDuration = 0;
	tokenPathTmp.leftTriphone = tokenPath[pos].modelId;
	//tokenPathTmp.leftChild = -1;
	//tokenPathTmp.rightChild = -1;
	tokenPathTmp.leftTone = leftPitch; //从INITIAL->FINAL,传递左边的声调
	tokenPathTmp.nextToken = -1;
			
	//find all possible right contexts
	linkNo = pLexicalTree[baseGrammar].linkNum;
	stNode = pLexicalTree[baseGrammar].startNode;
	assert(linkNo < 200);
	//only right phone and right pitch is dynamical
	for(iii = 0; iii < linkNo; iii++)
	{
		rightContext = stNode + iii;
		if(pLexicalTree[rightContext].startNode > 0)
		{			
			//extend the first final and right context is INITIAL
			hmmModelCode = pLexicalTree[rightContext].nodeId;
			rightPhone = mapping[hmmModelCode];		
			assert(rightPhone <= MAX_CODE_INITIAL_PART);
			//check all possible right tones of current right phone. Because right phone is INITIAL, detect the next layer
			//of current node representing right phone
			nextLinkNo = pLexicalTree[rightContext].linkNum;
			nextStNode = pLexicalTree[rightContext].startNode;
			for(jj = 0; jj < nextLinkNo; jj++)
			{
				node = nextStNode + jj;
				rightPitch = pLexicalTree[node].toneId;
				assert(mapping[pLexicalTree[node].nodeId] > MAX_CODE_INITIAL_PART && mapping[pLexicalTree[node].nodeId] != SILENCE_CODE);
				hmmModelCode = triphoneMap[basePhone][basePitch][leftPitch][rightPitch][leftPhone][rightPhone];
				//dynamically calculate LM predictive probability
				curProb = GetCacheProb(word1, word2, node, backWeight);
				//curProb = GetDisLmProb(word1, word2, node, backWeight);
				tokenPathTmp.modelId = hmmModelCode;
				tokenPathTmp.lmLookaheadScore = curProb;
				tokenPathTmp.lmScore = probLm + (curProb - probFather);
				//
				InsertPath(flag2, tokenPathTmp, 1);
			}
		}
		else
		{			
			curProb = GetCacheProb(word1, word2, rightContext, backWeight);
			//curProb = GetDisLmProb(word1, word2, rightContext, backWeight);
			assert(pLexicalTree[rightContext].nodeId == TRI_SILENCE_CODE);
			//
			tokenPathTmp.lmLookaheadScore = curProb;
			tokenPathTmp.lmScore = probLm + (curProb - probFather);
			//
			rightPhone = mapping[pLexicalTree[rightContext].nodeId];
			assert(rightPhone == SILENCE_CODE);
			for(jj = 0; jj <= PITCH_NULL; jj++)
			{
				hmmModelCode = triphoneMap[basePhone][basePitch][leftPitch][jj][leftPhone][rightPhone];
				tokenPathTmp.modelId = hmmModelCode;
				InsertPath(flag2, tokenPathTmp, 1);
			}
			//right context is the first INITIAL of the next word
			linkNoTmp = pLexicalTree[0].linkNum;
			stNodeTmp = pLexicalTree[0].startNode;
			assert(linkNoTmp <= 24);
			
			for(kk1 = 0; kk1 < linkNoTmp; kk1++)
			{
				rightContext = stNodeTmp + kk1;
				hmmModelCode = pLexicalTree[rightContext].nodeId;
				rightPhone = mapping[hmmModelCode];
				assert(rightPhone <= MAX_CODE_INITIAL_PART);
				//insert new path into path stack
				for(jj = 0; jj < 5; jj++)
				{
				//	rightPitch = firstPhonePitch[rightPhone][jj];
					if(!firstPhonePitch[rightPhone][jj])
						continue;
					hmmModelCode = triphoneMap[basePhone][basePitch][leftPitch][jj][leftPhone][rightPhone];
					tokenPathTmp.modelId = hmmModelCode;
					InsertPath(flag2, tokenPathTmp, 1);
				}				
			}
		}
	}
}
*/
void	FlyingExtendFinal(int pos, int baseGrammar, long backWeight)
{
	int linkNo, stNode, linkNoTmp, stNodeTmp, basePhone, leftPhone, leftContext, rightPhone, rightContext, iii, jj, kk1;
	long curProb;
	unsigned short word1, word2;
	char basePitch, leftPitch, rightPitch;
	CPhoneNode tokenPathTmp;

	//the phone ID currently extended
	basePhone = pLexicalTree[baseGrammar].nodeId; //current phone extended
	basePitch = pLexicalTree[baseGrammar].toneId; //current pitch 
	assert(basePhone > MAX_CODE_INITIAL_PART && basePhone != SILENCE_CODE);
	//
	leftContext = tokenPath[pos].grammarNode; 
	leftPhone = pLexicalTree[leftContext].nodeId; //left context phone of current phone
	leftPitch = tokenPath[pos].leftTone; //left context pitch 
	assert(leftPhone <= MAX_CODE_INITIAL_PART);

	word1 = tokenPath[pos].historyWord1;
	word2 = tokenPath[pos].historyWord2;
	//insert new path into path stack
	tokenPathTmp.amScore = tokenPath[pos].amScore;
	tokenPathTmp.grammarNode = baseGrammar;
	tokenPathTmp.historyWord1 = word1;
	tokenPathTmp.historyWord2 = word2;
	tokenPathTmp.preWordPos = tokenPath[pos].preWordPos;
	tokenPathTmp.startTime = tokenPath[pos].startTime;
	tokenPathTmp.tokenActive = 0;
	tokenPathTmp.wordDuration = 0;
	tokenPathTmp.leftTriphone = tokenPath[pos].modelId;
	tokenPathTmp.leftTone = leftPitch; //从INITIAL->FINAL,传递左边的声调
	tokenPathTmp.nextToken = -1;
			
	//find all possible right contexts
	linkNo = pLexicalTree[baseGrammar].linkNum;
	stNode = pLexicalTree[baseGrammar].startNode;
	assert(linkNo < 200);
	//
	//only right phone and right pitch is dynamical
	for(iii = 0; iii < linkNo; iii++)
	{
		rightContext = stNode + iii;
		if(pLexicalTree[rightContext].startNode > 0)
		{			
			//extend the first final and right context is INITIAL
			rightPhone = pLexicalTree[rightContext].nodeId;;		
			assert(rightPhone <= MAX_CODE_INITIAL_PART);
			rightPitch = pLexicalTree[rightContext].toneId;
			//dynamically calculate LM predictive probability
#ifdef LM_CACHE
			curProb = GetCacheProb(word1, word2, rightContext, backWeight);
#else
			curProb = GetDisLmProb(word1, word2, rightContext, backWeight);
#endif
			tokenPathTmp.modelId = triphoneMapFinal(basePhone, basePitch, leftPitch, rightPitch, leftPhone, rightPhone);
			tokenPathTmp.lmLookaheadScore = curProb;
			tokenPathTmp.lmScore = tokenPath[pos].lmScore + (curProb - tokenPath[pos].lmLookaheadScore);
			//
			if(ACOUSTIC_WEIGHT * tokenPathTmp.amScore + LANGUAGE_WEIGHT * tokenPathTmp.lmScore < maxProbThreshold - maxOffset)
				continue;

			InsertPath(flag2, tokenPathTmp, 1);
		}
		else
		{			
#ifdef LM_CACHE
			curProb = GetCacheProb(word1, word2, rightContext, backWeight);
#else
			curProb = GetDisLmProb(word1, word2, rightContext, backWeight);
#endif
			assert(pLexicalTree[rightContext].nodeId == SILENCE_CODE);
			//
			tokenPathTmp.lmLookaheadScore = curProb;
			tokenPathTmp.lmScore = tokenPath[pos].lmScore + (curProb - tokenPath[pos].lmLookaheadScore);
			//
			if(ACOUSTIC_WEIGHT * tokenPathTmp.amScore + LANGUAGE_WEIGHT * tokenPathTmp.lmScore < maxProbThreshold - maxOffset)
				continue;

			//rightPhone = mappingPitch[pLexicalTree[rightContext].nodeId];
			rightPhone = pLexicalTree[rightContext].nodeId;
			assert(rightPhone == SILENCE_CODE);
			for(jj = 0; jj <= PITCH_NULL; jj++)
			{
				tokenPathTmp.modelId = triphoneMapFinal(basePhone, basePitch, leftPitch, jj, leftPhone, rightPhone);
				InsertPath(flag2, tokenPathTmp, 1);
			}
			//right context is the first INITIAL of the next word
			linkNoTmp = pLexicalTree[0].linkNum;
			stNodeTmp = pLexicalTree[0].startNode;
			
			for(kk1 = 0; kk1 < linkNoTmp; kk1++)
			{
				rightContext = stNodeTmp + kk1;
				rightPhone = pLexicalTree[rightContext].nodeId;
				rightPitch = pLexicalTree[rightContext].toneId;
				assert(rightPhone <= MAX_CODE_INITIAL_PART);
				//insert new path into path stack
				tokenPathTmp.modelId = triphoneMapFinal(basePhone, basePitch, leftPitch, rightPitch, leftPhone, rightPhone);
				InsertPath(flag2, tokenPathTmp, 1);
			}
		}
	}
}

void	FlyingExtendInitial(int pos, int baseGrammar, long backWeight)
{
	int linkNo, stNode, basePhone, leftPhone, leftContext, rightPhone, rightContext, ii;
	long curProb;
	unsigned short word1, word2;
	CPhoneNode tokenPathTmp;

	leftContext = tokenPath[pos].grammarNode;
	//leftPhone = mappingPitch[pLexicalTree[leftContext].nodeId];
	leftPhone = pLexicalTree[leftContext].nodeId;
	assert(leftPhone > MAX_CODE_INITIAL_PART);
	//
	word1 = tokenPath[pos].historyWord1;
	word2 = tokenPath[pos].historyWord2;
	//insert new path into path stack			
	tokenPathTmp.amScore = tokenPath[pos].amScore;
	tokenPathTmp.historyWord1 = word1;
	tokenPathTmp.historyWord2 = word2;
	tokenPathTmp.preWordPos = tokenPath[pos].preWordPos;
	tokenPathTmp.startTime = tokenPath[pos].startTime;
	tokenPathTmp.tokenActive = 0;
	tokenPathTmp.wordDuration = 0;
	tokenPathTmp.leftTriphone = tokenPath[pos].modelId;
	tokenPathTmp.leftTone = pLexicalTree[leftContext].toneId; //SILENCE->INITIAL/FINAL-->INITIAL,更新声调,记录当前模型左边的声调			
	tokenPathTmp.nextToken = -1;
	tokenPathTmp.grammarNode = baseGrammar;

	//find all possible right contexts
	linkNo = pLexicalTree[baseGrammar].linkNum;
	stNode = pLexicalTree[baseGrammar].startNode;
	assert(linkNo < 200);
	//basePhone = mappingPitch[pLexicalTree[baseGrammar].nodeId];
	basePhone = pLexicalTree[baseGrammar].nodeId;
	assert(basePhone <= MAX_CODE_INITIAL_PART);
	//
	for(ii = 0; ii < linkNo; ii++)
	{
		rightContext = stNode + ii;
		//hmmModelCode = pLexicalTree[rightContext].nodeId;
		//rightPhone = mappingPitch[hmmModelCode];
		rightPhone = pLexicalTree[rightContext].nodeId;
		assert(rightPhone > MAX_CODE_INITIAL_PART && rightPhone != SILENCE_CODE);
		//
#ifdef LM_CACHE
		curProb = GetCacheProb(word1, word2, rightContext, backWeight);
#else
		curProb = GetDisLmProb(word1, word2, rightContext, backWeight);
#endif
		//
		tokenPathTmp.modelId = triphoneMapInitial(basePhone, 0, 0, 0, leftPhone, rightPhone);
		tokenPathTmp.lmScore = tokenPath[pos].lmScore + (curProb - tokenPath[pos].lmLookaheadScore);
		tokenPathTmp.lmLookaheadScore = curProb;

		if(ACOUSTIC_WEIGHT * tokenPathTmp.amScore + LANGUAGE_WEIGHT * tokenPathTmp.lmScore < maxProbThreshold - maxOffset)
			continue;
		InsertPath(flag2, tokenPathTmp, 1);
	}
}

void TreeLeafExtend(int pos, int newGrammar, long curProb)
{
	int leftContext, leftPhone;
	CPhoneNode tokenPathTmp;
	char leftPitch;

	leftContext = tokenPath[pos].grammarNode;
	leftPhone = pLexicalTree[leftContext].nodeId;
	leftPitch = pLexicalTree[leftContext].toneId;
	assert(leftPhone != SILENCE_CODE && leftPhone > MAX_CODE_INITIAL_PART);
	assert(pLexicalTree[newGrammar].nodeId == SILENCE_CODE);
	//insert new path into path stack
	tokenPathTmp.amScore = tokenPath[pos].amScore;
	tokenPathTmp.lmScore = tokenPath[pos].lmScore + (curProb - tokenPath[pos].lmLookaheadScore);
	tokenPathTmp.grammarNode = newGrammar;
	tokenPathTmp.historyWord1 = tokenPath[pos].historyWord1;
	tokenPathTmp.historyWord2 = tokenPath[pos].historyWord2;
	tokenPathTmp.lmLookaheadScore = curProb;
	tokenPathTmp.modelId = TRI_SILENCE_CODE;
	tokenPathTmp.preWordPos = tokenPath[pos].preWordPos;
	tokenPathTmp.startTime = tokenPath[pos].startTime;
	tokenPathTmp.tokenActive = 0;
	tokenPathTmp.wordDuration = 0;
	tokenPathTmp.leftTriphone = tokenPath[pos].modelId;
	tokenPathTmp.nextToken = -1;
	tokenPathTmp.leftTone = tokenPath[pos].leftTone; //从FINAL->SILENCE,传递前一音节的声调

	if(ACOUSTIC_WEIGHT * tokenPathTmp.amScore + LANGUAGE_WEIGHT * tokenPathTmp.lmScore
		< maxProbThreshold - maxOffset)
		return;
	InsertPath(flag2, tokenPathTmp, 1);	
}

void CollectWord(int time, int stackIdx, int pos, int gnode)
{
	int i, wordCode, leastIdx, grammar, counter, preTime, num, hmmModel, prePos, index;
	long probLeast, probTmp, scoreAm, scoreLm, probLm[200];
	unsigned short xWord, yWord, curWord, refWord1, refWord2;
	char leftTone, baseTone;

	grammar = tokenPath[pos].grammarNode;
	hmmModel = tokenPath[pos].modelId;
	refWord1 = tokenPath[pos].historyWord1;
	refWord2 = tokenPath[pos].historyWord2;
	leftTone = tokenPath[pos].leftTone;
	baseTone = pLexicalTree[grammar].toneId;
	//the number of homophones
	assert(pLexicalTree[gnode].startNode < 0);
	xWord = pLexicalTree[gnode].wordFirst;
	yWord = pLexicalTree[gnode].wordLast;
	num = yWord - xWord + 1;	
	assert(num < 200);

	//Get trigram probability,if not found, backup to bigram, otherwise backup to unigram
	probTmp = CheckTriBackoff(refWord1, refWord2);
	if(probTmp == neg_huge)
		probTmp = 0;

	for(i = 0; i < num; i++)
	{
		curWord = xWord + i;
		probLm[i] = CheckTrigram(refWord1, refWord2, curWord);

		if(probLm[i] == neg_huge)
		{
			//backup to bigram
			//check whether Bigram exist
			probLm[i] = CheckBigram(refWord2, curWord);

			if(probLm[i] == neg_huge)
			{
				//back up to unigram
				assert((*(unsigned short*)(pUnigram + curWord * 4L)) != NEG_LM);
				assert((*(unsigned short*)(pUnigram + refWord2 * 4L + 2)) != NEG_LM);
				probLm[i] = pLmCodebook[(*(unsigned short*)(pUnigram + curWord * 4L))] + pLmCodebook[(*(unsigned short*)(pUnigram + refWord2 * 4L + 2))];
			}
			probLm[i] += probTmp;
		}
	}
	//
	preTime = tokenPath[pos].startTime - 1;//ending time of previous word
	prePos = tokenPath[pos].preWordPos; //position of previous word

	scoreAm = tokenPath[pos].amScore;
	scoreLm = tokenPath[pos].lmScore;
	//
	for(i = 0; i < num; i++)
	{
		probLm[i] -= tokenPath[pos].lmLookaheadScore;
		probLm[i] += scoreLm;
	}

	for(wordCode = xWord; wordCode <= yWord; wordCode++)
	{
		counter = wordNumber[time];
		index = wordCode - xWord;
		//check whether this word with special context exist. If yes, keep better one
		for(i = 0; i < counter; i++)
		{
			if((wordLattice[time][i].wordId == wordCode)
				&&(wordLattice[time][i].hisWord2 == refWord2)
				&&(wordLattice[time][i].lastModel == hmmModel))
				{
					//compare the score and keep the better one
					if(ACOUSTIC_WEIGHT * scoreAm + LANGUAGE_WEIGHT * probLm[index] 
						> ACOUSTIC_WEIGHT * wordLattice[time][i].amScore + LANGUAGE_WEIGHT * wordLattice[time][i].lmScore)
					{
						wordLattice[time][i].amScore = scoreAm;
						wordLattice[time][i].lmScore = probLm[index];
						wordLattice[time][i].leftTriphone = tokenPath[pos].leftTriphone;
						wordLattice[time][i].preWordPos = tokenPath[pos].preWordPos;
						wordLattice[time][i].preTime = preTime;
						wordLattice[time][i].hisWord1 = refWord1;
						wordLattice[time][i].toneId = baseTone;
						wordLattice[time][i].leftTone = leftTone;
						wordLattice[time][i].modelDur = tokenPath[pos].wordDuration;
					}
					break;
				}
		}
		if(i < counter)
			continue;

		//if this word with special context is not in word-stack
		if(counter < MAX_WORD_CANDIDATES)
		{
			//add the word into word candidates
			wordLattice[time][counter].wordId = wordCode;
			wordLattice[time][counter].lastModel = hmmModel; //curPhone;
			wordLattice[time][counter].preTime = preTime;
			wordLattice[time][counter].preWordPos = prePos;
			wordLattice[time][counter].amScore = scoreAm;
			wordLattice[time][counter].lmScore = probLm[index];
			wordLattice[time][counter].hisWord1 = refWord1;
			wordLattice[time][counter].hisWord2 = refWord2;
			wordLattice[time][counter].leftTriphone = tokenPath[pos].leftTriphone;
			wordLattice[time][counter].toneId = baseTone;
			wordLattice[time][counter].leftTone = leftTone;
			wordLattice[time][counter].modelDur = tokenPath[pos].wordDuration;

			wordNumber[time]++;
		}
		else
		{
			//word-stack is full. find the least score, compare with current score and keep the better
			probLeast = pos_huge;
			leastIdx = -1;
			for(i = 0; i < counter; i++)
			{
				probTmp = ACOUSTIC_WEIGHT * wordLattice[time][i].amScore + LANGUAGE_WEIGHT * wordLattice[time][i].lmScore;
				if(probLeast > probTmp)
				{
					probLeast = probTmp;
					leastIdx = i;
				}
			}
			assert(leastIdx >= 0);
			//
			if(probLeast < ACOUSTIC_WEIGHT * scoreAm + LANGUAGE_WEIGHT * probLm[index])
			{
				wordLattice[time][leastIdx].wordId = wordCode;
				wordLattice[time][leastIdx].lastModel = hmmModel; //curPhone;
				wordLattice[time][leastIdx].preTime = preTime;
				wordLattice[time][leastIdx].preWordPos = prePos;
				wordLattice[time][leastIdx].amScore = scoreAm;
				wordLattice[time][leastIdx].lmScore = probLm[index];
				wordLattice[time][leastIdx].hisWord1 = refWord1;
				wordLattice[time][leastIdx].hisWord2 = refWord2;
				wordLattice[time][leastIdx].leftTriphone = tokenPath[pos].leftTriphone;
				wordLattice[time][leastIdx].toneId = baseTone;
				wordLattice[time][leastIdx].leftTone = leftTone;
				wordLattice[time][leastIdx].modelDur = tokenPath[pos].wordDuration;
			}
		}
	}
}


/*/bigram is sorted by probability
long GetDisLmProb(unsigned short wFirstID, unsigned short wSecondID, int childNode, long triBackWeight)
{
	unsigned short xWord, yWord;
	int ii, jj;
	long offSet, num, probLmMax, backWeight, probTmp;
	unsigned char *pTmp;

	// Get the Word-Range of childNode
	xWord = pLexicalTree[childNode].wordFirst;
	yWord = pLexicalTree[childNode].wordLast;

	//check whether trigram exist
	probLmMax = neg_huge;
	probLmMax = GetDisTrigram(wFirstID, wSecondID, xWord, yWord);

	if(probLmMax == neg_huge)
	{
		// Get the Max Bigram Within the Range
		offSet = *(long*)(pBigramIdx + wSecondID * 6L);
		num = *(unsigned short*)(pBigramIdx + wSecondID * 6L+ 4);

		pTmp = pBigram + offSet * 10L;


		for(jj = 0; jj < num; jj++)
		{
			ii = *(unsigned short*)(pTmp + jj * 10);
			if(ii >= xWord && ii <= yWord)
			{
				probLmMax = *(long*)(pTmp + jj * 10 + 2);
				break;
			}
		}

		if(probLmMax != neg_huge)
			return probLmMax + triBackWeight;
		else
		{
			backWeight = *(long*)(pUnigram + wSecondID * 8L + 4);
			probLmMax = pLexicalTree[childNode].maxUnigramProb + backWeight + triBackWeight;
			return probLmMax;
		}
	}
	else
		return probLmMax;
}
//
long CheckTriBackoff(unsigned short w1, unsigned short w2)
{
	int ii;
	long probLm, offset;
	unsigned short word2, lmNum;
	unsigned char *pTmp;

	//check whether Bigram exist
	offset = *(long*)(pBigramIdx + w1 * 6L);
	lmNum = *(unsigned short*)(pBigramIdx + w1 * 6L + 4);
	pTmp = pBigram + offset * 10L;
	probLm = neg_huge;
	for(ii = 0; ii < lmNum; ii++)
	{
		word2 = *(unsigned short*)(pTmp + ii * 10L);
		if(word2 == w2)
		{
			probLm = (*(long*)(pTmp + ii * 10L + 6));
			break;
		}
	}
	return probLm;
}
long CheckBigram(unsigned short w1, unsigned short w2)
{
	int ii;
	long probLm, offset;
	unsigned short word2, lmNum;
	unsigned char *pTmp;

	//check whether Bigram exist
	offset = *(long*)(pBigramIdx + w1 * 6L);
	lmNum = *(unsigned short*)(pBigramIdx + w1 * 6L + 4);
	pTmp = pBigram + offset * 10L;
	probLm = neg_huge;
	for(ii = 0; ii < lmNum; ii++)
	{
		word2 = *(unsigned short*)(pTmp + ii * 10L);
		if(word2 == w2)
		{
			probLm = (*(long*)(pTmp + ii * 10L + 2));
			break;
		}
	}
	return probLm;
}

*/
////////functions are called when bigram is sorted by word code

//bigram is sorted by word code
long GetDisLmProb(unsigned short wFirstID, unsigned short wSecondID, int childNode, long triBackWeight)
{
	unsigned short xWord, yWord;
	long  probLmMax, backWeight;

	// Get the Word-Range of childNode
	xWord = pLexicalTree[childNode].wordFirst;
	yWord = pLexicalTree[childNode].wordLast;

	//check whether trigram exist
	probLmMax = neg_huge;
#ifdef TRIGRAM_PREDICT
	probLmMax = GetDisTrigram(wFirstID, wSecondID, xWord, yWord);
#endif

	if(probLmMax == neg_huge)
	{
		// Get the Max Bigram Within the Range
		probLmMax = GetDisBigram(wSecondID, xWord, yWord);

		if(probLmMax != neg_huge)
			return probLmMax + triBackWeight;
		else
		{
			backWeight = pLmCodebook[*(unsigned short*)(pUnigram + wSecondID * 4L + 2)];
			probLmMax = pLexicalTree[childNode].maxUnigramProb + backWeight + triBackWeight;
			return probLmMax;
		}
	}
	else
		return probLmMax;
}

long CheckTriBackoff(unsigned short w1, unsigned short w2)
{
	long offSet, probLmMax;
	unsigned char   *pTmp;
	int num, low, high, mid;
	unsigned short nCurIdx2, vqWord;

	// Just check the most favorate one
	offSet = *(long*)(pBigramIdx + w1 * 6L);
	num = *(unsigned short*)(pBigramIdx + w1 * 6L+ 4);

	pTmp = pBigram + offSet * 6L;

	probLmMax = neg_huge;
	if(num > 0)
	{
		nCurIdx2 = *(unsigned short*)(pTmp);
		if(w2 < nCurIdx2)
		{
			//if(probLmMax == NEG_LM)//-19980049)
			//	probLmMax = neg_huge;
			return probLmMax;
		}
		nCurIdx2 = *(unsigned short*)(pTmp + (num-1)*6);
		if(w2 > nCurIdx2)
		{
			//if(probLmMax == NEG_LM)// -19980049)
			//	probLmMax = neg_huge;
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
			nCurIdx2 = *(unsigned short*)(pTmp + mid * 6);

			if (nCurIdx2 == w2)
			{
				vqWord = *(unsigned short*)(pTmp + mid * 6 + 4);
				if(vqWord == NEG_LM)
					probLmMax = neg_huge;
				else
					probLmMax = pLmCodebook[vqWord];
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
//	if(probLmMax == NEG_LM)//-19980049)
//		probLmMax = neg_huge;

	return probLmMax;
}

long CheckBigram(unsigned short w1, unsigned short w2)
{
	long offSet, probLmMax;
	unsigned char   *pTmp;
	int num, low, high, mid;
	unsigned short nCurIdx2, vqWord;

	// Just check the most favorate one
	offSet = *(long*)(pBigramIdx + w1 * 6L);
	num = *(unsigned short*)(pBigramIdx + w1 * 6L+ 4);

	pTmp = pBigram + offSet * 6L;

	probLmMax = neg_huge;
	if(num > 0)
	{
		nCurIdx2 = *(unsigned short*)(pTmp);
		if(w2 < nCurIdx2)
		{
			//if(probLmMax == NEG_LM)//-19980049)
			//	probLmMax = neg_huge;
			return probLmMax;
		}
		nCurIdx2 = *(unsigned short*)(pTmp + (num-1)*6);
		if(w2 > nCurIdx2)
		{
			//if(probLmMax == NEG_LM)//-19980049)
			//	probLmMax = neg_huge;
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
			nCurIdx2 = *(unsigned short*)(pTmp + mid * 6);

			if (nCurIdx2 == w2)
			{
				vqWord = *(unsigned short*)(pTmp + mid * 6 + 2);
				if(vqWord == NEG_LM)
					probLmMax = neg_huge;
				else
					probLmMax = pLmCodebook[vqWord];
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
//	if(probLmMax == NEG_LM)//-19980049)
//		probLmMax = neg_huge;

	return probLmMax;
}

/*
long GetDisBigramOld(unsigned short w1, unsigned short xWord, unsigned short yWord)
{
	long offSet, probLmMax, probTmp;
	unsigned char   *pTmp;
	int num, low, high, mid, index, lowLevel, highLevel;
	unsigned short nCurIdx2, rword, wdtmp;

	// Just check the most favorate one
	offSet = *(long*)(pBigramIdx + w1 * 6L);
	num = *(unsigned short*)(pBigramIdx + w1 * 6L+ 4);

	pTmp = pBigram + offSet * 6L;

	probLmMax = neg_huge;
	if(num > 0)
	{
		nCurIdx2 = *(unsigned short*)(pTmp);
		if(nCurIdx2 > yWord)
		{
			if(probLmMax == NEG_LM)//-19980049)
				probLmMax = neg_huge;
			return probLmMax;
		}

		nCurIdx2 = *(unsigned short*)(pTmp + (num-1)*6);
		if(nCurIdx2 < xWord)
		{
			if(probLmMax == NEG_LM)//-19980049)
				probLmMax = neg_huge;
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
			nCurIdx2 = *(unsigned short*)(pTmp + mid * 6);

			wdtmp = __min(mid + 1, num - 1);
			rword = *(unsigned short*)(pTmp + wdtmp*6);

			if(nCurIdx2 < xWord)
			{
				wdtmp = __min(mid + 1, num - 1);
				rword = *(unsigned short*)(pTmp + wdtmp*6);
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
			nCurIdx2 = *(unsigned short*)(pTmp + mid * 6);

			if(nCurIdx2 > yWord)
			{
				wdtmp = __max(mid-1, 0);
				rword = *(unsigned short*)(pTmp + wdtmp*6);
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
			nCurIdx2 = *(unsigned short*)(pTmp + index * 6);
			if(nCurIdx2 >= xWord && nCurIdx2 <= yWord)
			{
				probTmp = pLmCodebook[*(unsigned short*)(pTmp + index * 6 + 2)];
				if(probTmp > probLmMax)
					probLmMax = probTmp;
			}
			index++;
		}
	}
	if(probLmMax == NEG_LM)//-19980049)
		probLmMax = neg_huge;
	return probLmMax;
}
*/

long GetDisBigram(unsigned short w1, unsigned short xWord, unsigned short yWord)
{
	long offSet, probLmMax, prob;
	unsigned char   *pTmp;
	int num, low, high, mid;
	unsigned short nCurIdx1, nCurIdx2, vqWord;

	// Just check the most favorate one
	offSet = *(long*)(pBigramIdx + w1 * 6L);
	num = *(unsigned short*)(pBigramIdx + w1 * 6L+ 4);

	pTmp = pBigram + offSet * 6L;

	probLmMax = neg_huge;
	if(num > 0)
	{
		nCurIdx1 = *(unsigned short*)(pTmp);
		nCurIdx2 = *(unsigned short*)(pTmp + (num-1)*6);
		if(nCurIdx1 > yWord || nCurIdx2 < xWord)
		{
		//	if(probLmMax == -19980049)
		//		probLmMax = neg_huge;
			return neg_huge;
		}

		/*/changed by deng yong gang, binary search:
		//modified by zhang hong, because of the change of data structure.
		low = 0;
		high = num-1;
		//find the first idx2
		while( low <= high ) 
		{
			mid = (low + high) / 2;
			nCurIdx2 = *(unsigned short*)(pTmp + mid * 10);

			wdtmp = __min(mid + 1, num - 1);
			rword = *(unsigned short*)(pTmp + wdtmp*10);

			if(nCurIdx2 < xWord)
			{
				wdtmp = __min(mid + 1, num - 1);
				rword = *(unsigned short*)(pTmp + wdtmp*10);
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
			nCurIdx2 = *(unsigned short*)(pTmp + mid * 10);

			if(nCurIdx2 > yWord)
			{
				wdtmp = __max(mid-1, 0);
				rword = *(unsigned short*)(pTmp + wdtmp*10);
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
			nCurIdx2 = *(unsigned short*)(pTmp + index * 10);
			if(nCurIdx2 >= xWord && nCurIdx2 <= yWord)
			{
				probTmp = *(long*)(pTmp + index * 10 + 2);
				if(probTmp > probLmMax)
					probLmMax = probTmp;
			}
			index++;
		}
*/
		low = 0;
		high = num-1;
		while (low <= high)
		{
			mid = (low + high) / 2;
			nCurIdx2 = *(unsigned short*)(pTmp + mid * 6);

			if ((nCurIdx2 >= xWord) && (nCurIdx2 <= yWord))
			{
				vqWord = *(unsigned short*)(pTmp + mid * 6 + 2);
				if(vqWord == NEG_LM)
					probLmMax = neg_huge;
				else
					probLmMax = pLmCodebook[vqWord];
				//*maxWord = nCurIdx2;

				//向前顺序遍历
				low = mid - 1;
				nCurIdx2 = *(unsigned short*)(pTmp + low * 6);
				while ((low >= 0) && (nCurIdx2 >= xWord))
				{
					vqWord = *(unsigned short*)(pTmp + low * 6 + 2);
					if(vqWord == NEG_LM)
						prob = neg_huge;
					else
						prob = pLmCodebook[vqWord];
					if(probLmMax < prob)
					{
						probLmMax = prob;
						//*maxWord = nCurIdx2;
					}

					low--;
					nCurIdx2 = *(unsigned short*)(pTmp + low * 6);
				}

				//向后顺序遍历
				low = mid + 1;
				nCurIdx2 = *(unsigned short*)(pTmp + low * 6);
				while ((low <= num - 1) && (nCurIdx2 <= yWord))
				{
					vqWord = *(unsigned short*)(pTmp + low * 6 + 2);
					if(vqWord == NEG_LM)
						prob = neg_huge;
					else
						prob = pLmCodebook[vqWord];
					if (probLmMax < prob)
					{
						probLmMax = prob;
						//*maxWord = nCurIdx2;
					}

					low++;
					nCurIdx2 = *(unsigned short*)(pTmp + low * 6);
				}

				break;

			}
			if (nCurIdx2 > yWord)
				high = mid - 1;
			else
				low = mid + 1;
		}
	}
//	if(probLmMax == NEG_LM)//-19980049)
//		probLmMax = neg_huge;
	return probLmMax;
}

///////////////////////////


long CheckTrigram(unsigned short w1, unsigned short w2, unsigned short w3)
{
	unsigned char   *pTmp;
	long  offSet;
	int num;
	long word_ptr,Ptri;
	unsigned short vqWord;

	// Just check the most favorate one
	offSet = *(long*)(pTrigramIdx + w1 * 8);
	num = *(unsigned int*)(pTrigramIdx + w1 * 8 + 4);
	pTmp = pTrigram + offSet * 6;
	word_ptr = 0;

	//changed by deng yong gang, binary search:
	//modified by zhang hong, because of the change of data structure.
	int low = 0;
	int high = num-1;
	int mid;
	bool bFound = false;
	//find the first idx2
	Ptri = neg_huge;
	while( low <= high ) 
	{
		mid = (low + high) / 2;
		unsigned short nCurIdx2 = *(unsigned short*)(pTmp + mid * 6);
		unsigned short nCurIdx3 = *(unsigned short*)(pTmp + mid * 6 + 2);//add by zh

		if (nCurIdx2 == w2 && nCurIdx3 == w3)	
		{//edit by zh
			// found here!
			vqWord = *(unsigned short *)(pTmp+mid*6+4);
			if(vqWord == NEG_LM)
				Ptri = neg_huge;
			else
				Ptri=pLmCodebook[vqWord];
			//if(Ptri == NEG_LM)//-19980049)
			//	Ptri = neg_huge;
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

long GetDisTrigram(unsigned short w1, unsigned short w2, unsigned short xWord, unsigned short yWord)
{
	unsigned char   *pTmp;
	long  offSet;
	int num;
	long word_ptr,Ptri, probMax;

	// Just check the most favorite one
	offSet = *(long*)(pTrigramIdx + w1 * 8);
	num = *(unsigned int*)(pTrigramIdx + w1 * 8 + 4);
	pTmp = pTrigram + offSet * 6;
	word_ptr = 0;

	//changed by deng yong gang, binary search:
	//modified by zhang hong, because of the change of data structure.
	int low = 0;
	int high = num-1;
	int mid;
	bool bFound = 0;
	unsigned short nCurIdx2, nCurIdx3, vqWord;

//	PTri = neg_huge;
	//find the first idx2
	while( low <= high ) 
	{
		mid = (low + high) / 2;
		nCurIdx2 = *(unsigned short*)(pTmp + mid * 6);
		//unsigned short nCurIdx3 = *(unsigned short*)(pTmp + mid * 8 + 2);//add by zh

		if (nCurIdx2 == w2)
		{//edit by zh
			// found here!
//			Ptri=*(long *)(pTmp+mid*8+4);
			bFound = 1;
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
		return neg_huge;

	probMax = neg_huge;
	int index = mid;
	while(1)
	{
		nCurIdx2 = *(unsigned short*)(pTmp + index * 6);
		nCurIdx3 = *(unsigned short*)(pTmp + index * 6 + 2);
		if(nCurIdx2 != w2)
			break;
		if(nCurIdx3 < xWord)
			break;
		if(nCurIdx3 <= yWord)
		{	
			vqWord = *(unsigned short *)(pTmp + index * 6 + 4);
			if(vqWord == NEG_LM)
				Ptri = neg_huge;
			else
				Ptri = pLmCodebook[vqWord];
			if(Ptri > probMax)
				probMax = Ptri;
		}
		index--;
	}
	index = mid + 1;
	while(1)
	{
		nCurIdx2 = *(unsigned short*)(pTmp + index * 6);
		nCurIdx3 = *(unsigned short*)(pTmp + index * 6 + 2);
		if(nCurIdx2 != w2)
			break;
		if(nCurIdx3 > yWord)
			break;
		if(nCurIdx3 >= xWord)
		{
			vqWord = *(unsigned short *)(pTmp + index * 6 + 4);
			if(vqWord == NEG_LM)
				Ptri = neg_huge;
			else
				Ptri = pLmCodebook[vqWord];
			if(Ptri > probMax)
				probMax = Ptri;
		}
		index++;
	}
//	if(probMax == NEG_LM)//-19980049)
//		probMax = neg_huge;
	return probMax;
}       


int InsertPath(int stackIdx, CPhoneNode path, int type)
{
	int curIndex, preIndex, bucketSign, module, startNo, moduleValue, preIndexTmp, curIndexTmp,
		minKey, minIndex, minPreIndex, keyCt;
	int languageWeight;
	__int64  keyValue;
	long probMin, probTmp, probSum;

	switch(type)
	{
		case 0:
			languageWeight = LANGUAGE_WEIGHT;
			bucketSign = 0;
			module = MODULE_VALUE1;
			startNo = 0; 
			moduleValue = MODULE_VALUE1;
			break;
		case 1:
			//inter word or inter-model extention
			languageWeight = LANGUAGE_WEIGHT;
			bucketSign = MODULE_VALUE1;
			module = MODULE_VALUE2;
			startNo = MODULE_VALUE1; 
			moduleValue = MODULE_VALUE1 + MODULE_VALUE2;
			break;
		case 2:
			//inter word or inter-model extention
			languageWeight = LANGUAGE_WEIGHT;
			bucketSign = MODULE_VALUE1;
			module = MODULE_VALUE2;
			startNo = MODULE_VALUE1; 
			moduleValue = MODULE_VALUE1 + MODULE_VALUE2;
			break;
		default:
			return 0;
	}

	probSum = ACOUSTIC_WEIGHT * path.amScore + languageWeight * path.lmScore;
//	if(probSum < maxProbThreshold - maxOffset)
//		return 1;

	keyValue = (path.grammarNode * path.modelId + path.tokenActive) % module;
	curIndex = pBucketHead[stackIdx][keyValue+bucketSign];
	preIndex = -1;

	while(curIndex >= 0)
	{
		/*
		if(path.historyWord1 == tokenPath[curIndex].historyWord1
		  && path.historyWord2 == tokenPath[curIndex].historyWord2
		  && path.grammarNode == tokenPath[curIndex].grammarNode
		  && path.modelId == tokenPath[curIndex].modelId
		  && path.tokenActive == tokenPath[curIndex].tokenActive)
		*/
		if(path.grammarNode == tokenPath[curIndex].grammarNode)
			if(path.modelId == tokenPath[curIndex].modelId)
				if(path.tokenActive == tokenPath[curIndex].tokenActive)
					if(path.historyWord2 == tokenPath[curIndex].historyWord2)
						if(path.historyWord1 == tokenPath[curIndex].historyWord1)
							break;
		//
		preIndex = curIndex;
		curIndex = tokenPath[curIndex].nextToken;
	}

	if(curIndex >= 0)
	{
		if(probSum > ACOUSTIC_WEIGHT * tokenPath[curIndex].amScore + LANGUAGE_WEIGHT * tokenPath[curIndex].lmScore)
		{
			path.nextToken = tokenPath[curIndex].nextToken;
			tokenPath[curIndex] = path;	
			//计算最大概率
			if(maxProbThreshold < probSum)
				maxProbThreshold = probSum;
		}
	}
	else
	{ //this path is not in stack
		//apply a free block from the stack
		if(freeBlockTop >= NSLmax)
		{
			//find the path with minimal score and replace it with the current if current score is more than the minimal path
			probMin = pos_huge;
			minIndex = -1;
			for(keyCt = startNo; keyCt < moduleValue; keyCt++)
			{
				curIndexTmp = pBucketHead[stackIdx][keyCt];
				preIndexTmp = -1;
				while(curIndexTmp >= 0)
				{
					probTmp = ACOUSTIC_WEIGHT * tokenPath[curIndexTmp].amScore + LANGUAGE_WEIGHT * tokenPath[curIndexTmp].lmScore;
					if(probTmp < probMin)
					{
						probMin = probTmp;
						minIndex = curIndexTmp;
						minKey = keyCt;
						minPreIndex = preIndexTmp;
					}
					preIndexTmp = curIndexTmp;
					curIndexTmp = tokenPath[curIndexTmp].nextToken;
				}

			}
			assert(minIndex >= 0);
			if(probSum > probMin)
				ReleaseBlock(stackIdx, minKey, minPreIndex, minIndex);
			else
				return 1;
		}
		assert(freeBlockTop < NSLmax);
		assert(freeBlockIdx[freeBlockTop] != -1);
		if(pBucketHead[stackIdx][keyValue+bucketSign] != curIndex)
		{
			//the hash table is not empty and modify the last item and make nextToken point to new path
			assert(tokenPath[preIndex].nextToken == -1);
			tokenPath[preIndex].nextToken = freeBlockIdx[freeBlockTop];
		}
		else
		{
			assert(pBucketHead[stackIdx][keyValue+bucketSign] == -1);
			pBucketHead[stackIdx][keyValue+bucketSign] = freeBlockIdx[freeBlockTop];
		}
		//
		tokenPath[freeBlockIdx[freeBlockTop]] = path;
		freeBlockIdx[freeBlockTop] = -1;
		freeBlockTop++;
		assert(freeBlockTop <= NSLmax);
		//
		if(maxProbThreshold < probSum)
			maxProbThreshold = probSum;
	}
	return 1;
}

void	ReleaseBlock(int stackIdx, int keyCt, int preIndex, int curIndex)
{
	freeBlockTop--;
	assert(freeBlockTop >= 0);
	freeBlockIdx[freeBlockTop] = curIndex;
	//
	if(pBucketHead[stackIdx][keyCt] == curIndex)
		pBucketHead[stackIdx][keyCt] = tokenPath[curIndex].nextToken;
	else
	{
		assert(preIndex >= 0);
		tokenPath[preIndex].nextToken = tokenPath[curIndex].nextToken;
	}
}

long GetCacheProb(unsigned short w1, unsigned short w2, int node, long triBackWeight)
{
	int i, cacheNum, curHead;
	long curProb;

	curHead = pLmCacheHead[node];
	if(lmCacheFull[node])
		cacheNum = MAX_CACHE_LIST;
	else
		cacheNum = curHead;

	for(i = 0; i < cacheNum; i++)
#ifdef TRIGRAM_PREDICT
		if(lmCache[node][i].w1 == w1 && lmCache[node][i].w2 == w2)
#else
		if(lmCache[node][i].w2 == w2)
#endif
			break;

	if(i == cacheNum)
	{
		//not find in the cache
		curProb = GetDisLmProb(w1, w2, node, triBackWeight);
		lmCache[node][curHead].w1 = w1;
		lmCache[node][curHead].w2 = w2;
		lmCache[node][curHead].prob = curProb;
		pLmCacheHead[node]++;
		if(pLmCacheHead[node] == MAX_CACHE_LIST)
		{
			lmCacheFull[node] = 1;
			pLmCacheHead[node] = 0;
		}
	}
	else
		curProb = lmCache[node][i].prob;

	return curProb;
}

inline int triphoneMapInitial(short base, short baseTone, short leftTone, short rightTone, short left, short right)
{
	assert(base != SILENCE_CODE);
	left -= TOTAL_INITIAL;
	right -= TOTAL_INITIAL;
	return *(pTriphoneMapInitial + 
			        base * (TOTAL_FINAL + 1) * TOTAL_FINAL + 
					left * TOTAL_FINAL + 
					right);
}
inline int triphoneMapFinal(short base, short baseTone, short leftTone, short rightTone, short left,short  right)
{
	assert(base!=61);
	base -= TOTAL_INITIAL;
	right %= TOTAL_FINAL;
	return *(pTriphoneMapFinal + 
			base * TOTAL_INITIAL * (TOTAL_INITIAL + 1) * 5 * 6 * 6 + 
			baseTone * TOTAL_INITIAL * (TOTAL_INITIAL + 1) * 6 * 6 + 
			leftTone * TOTAL_INITIAL * (TOTAL_INITIAL + 1) * 6 + 
			rightTone * TOTAL_INITIAL * (TOTAL_INITIAL + 1) +
			left * (TOTAL_INITIAL + 1) + 
			right);
}


void CrossWordExtend(int tt)
{
	//declare local variables
	int jj, fatherContext, leftBasePhone, linkNo, nextLinkNo, hmmModelCode2, basePhone, ii_g, jj_g;
	unsigned short word1, word2;
	long probAm, probLm, triBackWeight, i_g, j_g, stNode, nextStNode, probSon;
	char leftTone, leftBaseTone, rightBaseTone;
	short leftPhone, rightPhone;
	CPhoneNode tokenPathTmp;

	//inter-word extension and extend word in word-lattice to the first level of lexical tree
	linkNo = pLexicalTree[0].linkNum;
	stNode = pLexicalTree[0].startNode;

	for(ii_g = 0; ii_g < wordNumber[tt]; ii_g++)
	{
		fatherContext = wordLattice[tt][ii_g].leftTriphone;
		leftBasePhone = mapping[fatherContext];
		leftBaseTone = wordLattice[tt][ii_g].leftTone; //get left tone
		//
		hmmModelCode2 = wordLattice[tt][ii_g].lastModel;
		leftPhone = mapping[hmmModelCode2];
		leftTone = wordLattice[tt][ii_g].toneId; //get self-tone
		//
		if(wordLattice[tt][ii_g].preTime < 0)
			word1 = HEAD_WORD;
		else
			word1 = wordLattice[tt][ii_g].hisWord2;

		word2 = wordLattice[tt][ii_g].wordId;
		probAm = wordLattice[tt][ii_g].amScore;
		probLm = wordLattice[tt][ii_g].lmScore;

		if(ACOUSTIC_WEIGHT * probAm + LANGUAGE_WEIGHT * probLm < maxProbThreshold - maxOffset)
			continue;
		//	
		triBackWeight = CheckTriBackoff(word1, word2);
		if(triBackWeight == neg_huge)
			triBackWeight = 0;

		for(jj = 0; jj < linkNo; jj++)
		{
			i_g = jj + stNode;
			//basePhone = mappingPitch[pLexicalTree[i_g].nodeId];
			basePhone = pLexicalTree[i_g].nodeId;
			//find all possible right conext
			nextLinkNo = pLexicalTree[i_g].linkNum;
			nextStNode = pLexicalTree[i_g].startNode;

			if(leftPhone != SILENCE_CODE)
			{
				//left phone must be FINAL
				assert(leftPhone > MAX_CODE_INITIAL_PART);
				rightBaseTone = pLexicalTree[i_g].toneId;
				if(triphoneMapFinal(leftPhone, leftTone, leftBaseTone, rightBaseTone, leftBasePhone, basePhone) != hmmModelCode2)
					continue;
			}

			for(jj_g = 0; jj_g < nextLinkNo; jj_g++)
			{
				j_g = nextStNode + jj_g;
				//rightPhone = mappingPitch[pLexicalTree[j_g].nodeId];
				rightPhone = pLexicalTree[j_g].nodeId;
				assert(rightPhone > MAX_CODE_INITIAL_PART && rightPhone != SILENCE_CODE);

				assert(basePhone <= MAX_CODE_INITIAL_PART);
				//dynamically calculate the predictive lM score
#ifdef LM_CACHE
				probSon = GetCacheProb(word1, word2, j_g, triBackWeight);
#else
				probSon = GetDisLmProb(word1, word2, j_g, triBackWeight);
#endif
				//new path
				tokenPathTmp.amScore = probAm;
				tokenPathTmp.lmScore = probLm + probSon + PENALTY; 
				tokenPathTmp.grammarNode = i_g; 
				tokenPathTmp.historyWord1 = word1;
				tokenPathTmp.historyWord2 = word2;
				tokenPathTmp.lmLookaheadScore = probSon;
				tokenPathTmp.modelId = triphoneMapInitial(basePhone, 0, 0, 0, leftPhone, rightPhone); 
				tokenPathTmp.preWordPos = ii_g;
				tokenPathTmp.startTime = tt + 1;
				tokenPathTmp.wordDuration = 0;
				tokenPathTmp.tokenActive = 0;
				tokenPathTmp.leftTriphone = hmmModelCode2;
				tokenPathTmp.nextToken = -1;
				
				if(leftPhone == SILENCE_CODE && wordLattice[tt][ii_g].modelDur >= 4)
					tokenPathTmp.leftTone = PITCH_NULL;
				else
					tokenPathTmp.leftTone = leftTone;
				//
				if(ACOUSTIC_WEIGHT * tokenPathTmp.amScore + LANGUAGE_WEIGHT * tokenPathTmp.lmScore 
					< maxProbThreshold - maxOffset)
					continue;

				InsertPath(flag2, tokenPathTmp, 2);
			}
		}
	}
}


void SortLattice(int time)
{
	int i, j, index;
	long scoreTmp;
	CWordLattice	tmp;

	for(i = 0; i < wordNumber[time] - 1; i++)
	{
		scoreTmp = ACOUSTIC_WEIGHT * wordLattice[time][i].amScore + LANGUAGE_WEIGHT * wordLattice[time][i].lmScore;
		index = i;
		for(j = i + 1; j < wordNumber[time]; j++)
		{
			if(scoreTmp < ACOUSTIC_WEIGHT * wordLattice[time][j].amScore + LANGUAGE_WEIGHT * wordLattice[time][j].lmScore)
			{
				scoreTmp = ACOUSTIC_WEIGHT * wordLattice[time][j].amScore + LANGUAGE_WEIGHT * wordLattice[time][j].lmScore;
				index = j;
			}
		}
		tmp = wordLattice[time][i];
		wordLattice[time][i] = wordLattice[time][index];
		wordLattice[time][index] = tmp;
	}			
}


short FindCodebook(float *pVec)
{
	int		i, minIdx, l;
	float	diffTemp, ftmp, dis, minDis;

	minIdx = -1;
	minDis = pos_huge;
	for(i = 0; i < CODE_BOOK_SIZE; i++)
	{
		dis = 0;
		for(l = 0; l < No_Dim; l++)  // Distance Calculation Option
		{
			diffTemp = pVec[l] - clusterMean[i*No_Dim+l];
			ftmp = diffTemp * diffTemp;
		//	dis += ftmp * clusterVar[i*No_Dim+l];
			dis += ftmp;
		}
		if(dis < minDis)
		{
			minDis = dis;
			minIdx = i;
		}
	}
	assert(minIdx >= 0);
	return minIdx;
}

void	IntraHmmExtend(int tt)
{
	int	phnidx, keyCt, curIndex, preIndex, jj;
	unsigned short j, i_s, j_s, word1, word2;
	long	probAm, probLm, iaal, i_g, ibbl;
	short	codeWord;//, preCodeWord=-1, sameCode=0;
	CPhoneNode tokenPathTmp;


//printf("%f\n",dis);
//	FILE *fp=fopen("debug.txt", "a+t");
//	fprintf(fp, "%-4d%f\n", tt, dis);
//	fclose(fp);

#ifdef FAST_COMPUTE
	codeWord = FindCodebook(pVector[tt]);
	//printf("%d\n",codeWord);
	//if(codeWord != preCodeWord)
	//{
	for(jj = 0; jj < No_Output; jj++)
		ProbBuffer[jj] = neg_huge; 
	//	preCodeWord = codeWord;
	//	sameCode=0;
	/*/}
	else
	{
		sameCode++;
		if(sameCode == 2)
		{
			for(jj = 0; jj < No_Output; jj++)
				ProbBuffer[jj] = neg_huge; 
			preCodeWord = codeWord;
			sameCode = 0;
		}
	}
	*/
#else
	for(jj = 0; jj < No_Output; jj++)
		ProbBuffer[jj] = neg_huge; 
#endif
		
	//Path extension in intra-word
	for(keyCt = 0; keyCt < MODULE_VALUE1 + MODULE_VALUE2; keyCt++)
	{
		curIndex = pBucketHead[flag1][keyCt];
		preIndex = -1;

		while(curIndex >= 0)
		{
			i_s = tokenPath[curIndex].tokenActive;
			i_g = tokenPath[curIndex].grammarNode; 
			phnidx = tokenPath[curIndex].modelId;			
			assert(i_s < Hmm[phnidx].No_State - 1);

			word1 = tokenPath[curIndex].historyWord1;
			word2 = tokenPath[curIndex].historyWord2;

			//extend path in intra-model
  			for(j = 0; j < Hmm[phnidx].nj[i_s]; j++)
			{			  
				j_s = Hmm[phnidx].indj[i_s * Hmm[phnidx].No_State + j]; //next HMM state
				//Compute AM score
				iaal = GetTransProb(phnidx, i_s, j_s);
#ifdef FAST_COMPUTE
				ibbl = GetObserveProbFast(phnidx, i_s, j_s, pVector[tt], codeWord);
#else
				ibbl = GetObserveProb(phnidx, i_s, j_s, pVector[tt]);
#endif
				probAm = tokenPath[curIndex].amScore + iaal + ibbl;
				probLm = tokenPath[curIndex].lmScore;

				//Check whether readily extended path exist
				tokenPathTmp.amScore = probAm;
				tokenPathTmp.lmScore = probLm;
				tokenPathTmp.grammarNode = i_g;
				tokenPathTmp.historyWord1 = word1;
				tokenPathTmp.historyWord2 = word2;
				tokenPathTmp.lmLookaheadScore = tokenPath[curIndex].lmLookaheadScore;
				tokenPathTmp.modelId = phnidx;
				tokenPathTmp.preWordPos = tokenPath[curIndex].preWordPos;
				tokenPathTmp.startTime = tokenPath[curIndex].startTime;
				tokenPathTmp.wordDuration = tokenPath[curIndex].wordDuration + 1;
				tokenPathTmp.tokenActive = j_s;//j_psf;
				tokenPathTmp.leftTriphone = tokenPath[curIndex].leftTriphone;
				tokenPathTmp.leftTone = tokenPath[curIndex].leftTone;
				tokenPathTmp.nextToken = -1;
				//
				if(ACOUSTIC_WEIGHT * tokenPathTmp.amScore + LANGUAGE_WEIGHT * tokenPathTmp.lmScore
					< maxProbThreshold - maxOffset)
					continue;

				InsertPath(flag2, tokenPathTmp, 0);				
			}
			ReleaseBlock(flag1, keyCt, preIndex, curIndex);
			preIndex = curIndex;
			curIndex = tokenPath[curIndex].nextToken;
		}
	}
}

// Process paths which reach the last state of Hmm Model and extend path to next model
// There are 2 cases: 
// 1. inter-model extension of intra-word 2. reach word's leaf and word can be determined, and extend
// path to next new word.
void	InterHmmExtend(int tt)
{
	int phnidx, jj, fatherContext, leftBasePhone, rightBasePhone, isCollectWord, 
		keyCt, curIndex, preIndex, linkNo, basePhone, temp, jj_g;
	unsigned short i_s, word1, word2;
	short leftPhone;
	long   probTmp, triBackWeight, i_g, j_g, stNode;
	char leftTone, leftBaseTone, rightBaseTone;

	for(keyCt = 0; keyCt < MODULE_VALUE1; keyCt++)
	{
		curIndex = pBucketHead[flag2][keyCt];
		preIndex = -1;

		while(curIndex >= 0)
		{
			i_g = tokenPath[curIndex].grammarNode;
			i_s = tokenPath[curIndex].tokenActive;
			phnidx = tokenPath[curIndex].modelId;
			if(i_s != Hmm[phnidx].No_State - 1)
			{
				preIndex = curIndex;
				curIndex = tokenPath[curIndex].nextToken;
				continue;		
			}
			//if this path reachs last state of HMM and is activated. First deactivated this path, then move to next node and HMM
			tokenPath[curIndex].tokenActive = 0;
			//
			leftPhone = pLexicalTree[i_g].nodeId; 
			leftTone = pLexicalTree[i_g].toneId; 
			assert(leftPhone == mapping[phnidx]);
			//
			word1 = tokenPath[curIndex].historyWord1;
			word2 = tokenPath[curIndex].historyWord2;
			//
			leftBasePhone = mapping[tokenPath[curIndex].leftTriphone];
			leftBaseTone = tokenPath[curIndex].leftTone; //get left tone of this model
			//
			triBackWeight = CheckTriBackoff(word1, word2);
			if(triBackWeight == neg_huge)
				triBackWeight = 0;

			//find the successors of current node
			linkNo = pLexicalTree[i_g].linkNum;
			stNode = pLexicalTree[i_g].startNode;
			//check the type of node
			if(stNode >= 0) 
			{
				//intra-word transition. tree node of current path is not a leaf
				assert(stNode != 0);
				for(jj_g = 0; jj_g < linkNo; jj_g++)
				{
					//extend all successors of current grammar node i_g
					j_g = stNode + jj_g; //extended successor node
					//hmmModelCode = pLexicalTree[j_g].nodeId;
					//rightBasePhone = mappingPitch[hmmModelCode];
					rightBasePhone= pLexicalTree[j_g].nodeId;
					rightBaseTone = pLexicalTree[j_g].toneId;
					//check type of extended node
					if(rightBasePhone <= MAX_CODE_INITIAL_PART) 
					{
						if(leftPhone != SILENCE_CODE)
							if(triphoneMapFinal(leftPhone, leftTone, leftBaseTone, rightBaseTone, leftBasePhone, rightBasePhone) != phnidx)
								continue;
						FlyingExtendInitial(curIndex, j_g, triBackWeight); //stack index1, stack index2, grammar node(base phone), right grammar
					}
					else if(rightBasePhone <= MAX_CODE_FINAL_PART)
					{
						//current node j_g is virtual model code and must dynamically extend all its possible triphones.
						//tree node ID is tonal base phone. fan-out all possible triphone with the same base phone
						//the Virtual node is FINAL 
						assert(leftPhone <= MAX_CODE_INITIAL_PART);
						assert(leftBasePhone > MAX_CODE_INITIAL_PART);
						assert(rightBasePhone > MAX_CODE_INITIAL_PART && rightBasePhone <= MAX_CODE_FINAL_PART);
						//Check the right context corresponding to node 'j_g'
						if(triphoneMapInitial(leftPhone, 0, 0, 0, leftBasePhone, rightBasePhone) != phnidx)
							continue;
						FlyingExtendFinal(curIndex, j_g, triBackWeight); //stack index1, grammar node(base phone), backoff weight
					}
					else
					{
						//reach the last state of last phone of a word and word can be determined. Collect word
						isCollectWord = 0;
						assert(leftPhone > MAX_CODE_INITIAL_PART && leftPhone <= MAX_CODE_FINAL_PART);
						assert(leftBasePhone <= MAX_CODE_INITIAL_PART);
						assert(rightBasePhone == SILENCE_CODE);
						for(jj = 0; jj <= PITCH_NULL; jj++)
						{
							if(triphoneMapFinal(leftPhone, leftTone, leftBaseTone, jj, leftBasePhone, rightBasePhone) == phnidx)							
							{
#ifdef LM_CACHE
								probTmp = GetCacheProb(word1, word2, j_g, triBackWeight);
#else
								probTmp = GetDisLmProb(word1, word2, j_g, triBackWeight);
#endif
								TreeLeafExtend(curIndex, j_g, probTmp); //silence is in the context set of current path. then extend current path to j_g node
								isCollectWord++;
								break;
							}
						}
						//
						isCollectWord = 0;
						fatherContext = pLexicalTree[0].startNode;
						for(jj = 0; jj < pLexicalTree[0].linkNum; jj++)
						{
							basePhone = pLexicalTree[fatherContext+jj].nodeId;
							temp = pLexicalTree[fatherContext+jj].toneId;
							if(triphoneMapFinal(leftPhone, leftTone, leftBaseTone, temp, leftBasePhone, basePhone) == phnidx)
							{
								CollectWord(tt, flag2, curIndex, j_g);
								break;
							}
						}
					}
				}
			}
			else
			{
				//readch the leaf of a word. tree node of current path is a leaf.
				assert(pLexicalTree[i_g].nodeId == SILENCE_CODE);
				CollectWord(tt, flag2, curIndex, i_g);
			}
			//release this block
			ReleaseBlock(flag2, keyCt, preIndex, curIndex);
			preIndex = preIndex;
			curIndex = tokenPath[curIndex].nextToken;
		}		
	}
}

void	TraceBack()
{
	unsigned short word1, word2;
	long probMax, probTmp, triBackWeight;
	int tt, jj, temp;

	for(jj = 0; jj < wordNumber[speechDataLength-1]; jj++)
	{
		word1 = wordLattice[speechDataLength-1][jj].wordId;
		word2 = wordLattice[speechDataLength-1][jj].hisWord2;
		probTmp = CheckTrigram(word2, word1, TAIL_WORD);
		if(probTmp == neg_huge)
		{
			triBackWeight = CheckTriBackoff(word2, word1);
			if(triBackWeight == neg_huge)
				triBackWeight = 0;

			probTmp = CheckBigram(word1, TAIL_WORD);
			if(probTmp == neg_huge)
			{
				probTmp = pLmCodebook[(*(unsigned short*)(pUnigram + TAIL_WORD * 4))] + pLmCodebook[(*(unsigned short*)(pUnigram + word1 * 4 + 2))];
			}
			probTmp += triBackWeight;
		}
		wordLattice[speechDataLength-1][jj].lmScore += probTmp;
	}

	int i, bestIdx, backTime, backPos;
	//
	/*FILE *fp;
	//fp=fopen("prob.txt", "a+t");
	//fprintf(fp, "%d  %d %d\n", wordLattice[speechDataLength-1][bestIdx].amScore, wordLattice[speechDataLength-1][bestIdx].lmScore,probMax);
	
	fp=fopen("debug.txt", "wt");
	for(i=0;i<speechDataLength;i++)
	{
		fprintf(fp, "%d\n", i);
		for(int iii=0;iii<MAX_WORD_CANDIDATES;iii++)
		{
			fprintf(fp, "%4d ID= %d w1= %d w2= %d pre= %d preTime= %d model= %d left = %d tone= %d lTone= %d am= %d lm= %d sum= %d\n",
				iii, wordLattice[i][iii].wordId,wordLattice[i][iii].hisWord1,wordLattice[i][iii].hisWord2,
				wordLattice[i][iii].preWordPos,wordLattice[i][iii].preTime,
				wordLattice[i][iii].lastModel, wordLattice[i][iii].leftTriphone, 
				wordLattice[i][iii].toneId, wordLattice[i][iii].leftTone, 
				wordLattice[i][iii].amScore,wordLattice[i][iii].lmScore,
				ACOUSTIC_WEIGHT*wordLattice[i][iii].amScore+LANGUAGE_WEIGHT*wordLattice[i][iii].lmScore);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
*/
	//trace back
	probMax = neg_huge;
	bestIdx = -1;
	//trace back
	for(tt = speechDataLength-1; tt >= 0; tt--)
	{
		if(wordNumber[tt] > 0)
		{
			for(i = 0; i < wordNumber[tt]; i++)
			{
				probTmp = wordLattice[tt][i].amScore * ACOUSTIC_WEIGHT + LANGUAGE_WEIGHT * wordLattice[tt][i].lmScore;
				if(probTmp > probMax)
				{
					probMax = probTmp;
					bestIdx = i;
				}
			}
			backTime = tt;
			break;
		}
	}

	if(bestIdx == -1)
	{
		printf("no sentence outputted\n");
		senLen = 0;
		return;
	}

	senLen = 0;
	wordSeq[senLen] = wordLattice[backTime][bestIdx].wordId;
	timeSeq[senLen] = backTime;
	posSeq[senLen] = bestIdx;
	//
	senLen++;
	backPos = wordLattice[backTime][bestIdx].preWordPos;
	backTime = wordLattice[backTime][bestIdx].preTime;

	while(1)
	{
		if(backTime <= 0)
			break;

		wordSeq[senLen] = wordLattice[backTime][backPos].wordId;
		timeSeq[senLen] = backTime;
		posSeq[senLen] = backPos;
		//
		temp = backTime;
		backTime = wordLattice[temp][backPos].preTime;
		backPos = wordLattice[temp][backPos].preWordPos;
		senLen++;
	}
	//
}

void	InitPath()
{
	long rightContext, i_g, stNode, nextStNode;
	int linkNo, nextLinkNo, hmmModelCode, basePhone, ii_g, jj_g;
	short leftPhone, rightPhone;
	CPhoneNode tokenPathTmp;

	// Initialization of Search Stack. The sentence maybe begin with silence or non-silence. Assume left context of first
	// phone is silence
	flag1 = 0;                
	flag2 = 1;                
	maxProbThreshold = neg_huge;

	// Maybe there is silence  before speech
	tokenPathTmp.grammarNode = 0;
	tokenPathTmp.leftTriphone = TRI_SILENCE_CODE;//-1;//TRI_SILENCE_CODE + 1; 
	tokenPathTmp.leftTone = PITCH_NULL;
	tokenPathTmp.modelId = TRI_SILENCE_CODE;
	tokenPathTmp.tokenActive = 0;
	tokenPathTmp.amScore = 0;
	tokenPathTmp.lmScore = 0;
	tokenPathTmp.lmLookaheadScore = 0;
	tokenPathTmp.historyWord1 = HEAD_WORD;
	tokenPathTmp.historyWord2 = HEAD_WORD;
	tokenPathTmp.preWordPos = -1;
	tokenPathTmp.startTime = 0;
	tokenPathTmp.wordDuration = 0; //to debug
	tokenPathTmp.nextToken = -1;

	InsertPath(flag1, tokenPathTmp, 0);
	// Extend all child nodes of root node
	leftPhone = pLexicalTree[0].nodeId; 
	//
	linkNo = pLexicalTree[0].linkNum; 
	stNode = pLexicalTree[0].startNode;
	//
	for(ii_g = 0; ii_g < linkNo; ii_g++)
	{
		//find all posible right context which are all child nodes of this node, and flying extension
		i_g = stNode + ii_g; //node readily to be extended
		basePhone = pLexicalTree[i_g].nodeId;
		assert(basePhone <= MAX_CODE_INITIAL_PART);

		nextLinkNo = pLexicalTree[i_g].linkNum; 
		nextStNode = pLexicalTree[i_g].startNode; 
		for(jj_g = 0; jj_g < nextLinkNo; jj_g++)
		{
			rightContext = nextStNode + jj_g; //grammar node
			rightPhone = pLexicalTree[rightContext].nodeId;
			hmmModelCode = triphoneMapInitial(basePhone, 0, 0, 0, leftPhone, rightPhone);
			assert(hmmModelCode >= 0);
				
			//check whether the path exists.If it exits, return 1;otherwise, return 0
			//grammar and model identities can confirm a path.If the grammar node is neither first nor
			//last phone, only need grammar node to determine a new path.Otherwise, need grammar and mode identity.
			//this path is not in path-stack and insert it into stack
			tokenPathTmp.grammarNode = i_g;
			tokenPathTmp.leftTriphone = TRI_SILENCE_CODE;
			tokenPathTmp.modelId = hmmModelCode;
			tokenPathTmp.historyWord1 = HEAD_WORD;
			tokenPathTmp.historyWord2 = HEAD_WORD;
			tokenPathTmp.amScore = 0;
			tokenPathTmp.lmScore = 0;
			tokenPathTmp.lmLookaheadScore = 0;
			tokenPathTmp.preWordPos = -1;
			tokenPathTmp.wordDuration = 0;
			tokenPathTmp.startTime = 0;
			tokenPathTmp.tokenActive = 0;
			tokenPathTmp.leftTone = PITCH_NULL;
			tokenPathTmp.nextToken = -1;
			//
			InsertPath(flag1, tokenPathTmp, 0);
		}		
	}
}
/////////////////////////////////////////////////////////////////////////
//这个程序是进行识别过程的主程序，在这个程序中以搜索的路径节点为中心
//进行语言和声学模型的扩展
//pVector is the vector of specified speech which was extracted by a series of method.
//Thr parameter "speechDataLength" is the true length of pvector. That is the demension 
//of pVector is speechDataLength to product NO_DIM.
///////////////////////////////////////////////////// wsj July 21, 2000


#include "stdafx.h"
#include "const.h"
#include "global.h"
#include "recognize.h"


#define INCREMENTAL_SEARCH

int	timeAxis = 0;

extern char  m_firmWords[MAX_SENTENCE_CHARACTERS];

void	OnepassSearchEngineInit()
{
	timeAxis = 0;
	//路径初始化，变量复位.
	g_pWordLattice->ResetWordCounter(); //高升编辑
	g_pPath->Init( );
	
	g_pLextree->BeginInitPathNode();//进行零节点的扩展	

	g_pPath->m_lProbMax = NEG_HUGE;		
	//Nov.2000
	g_pAM->m_nextCodeWord = -1;
	g_pAM->ResetAmCacheBuffer(); //Nov.3,2000
}

void	OnepassSearchEngine(float *pVector, short fifoNull, short sentenceEnd)
{
	//WORD	t;
	int	num, timeTmp;
	int	keyCt, nextKeyKeep;
	int	nNodes;
	PHONENODE	SeedNode;		//As a seed node was transmitted to Lextree or HMM to expand and get more nodes.
								//the nNodes is the number of the nodes which were gotten from the SeedNode.
	int CurIndex, preIndex;

	g_pPath->m_lProbMax = NEG_HUGE;//-= (ACOUSTIC_WEIGHT * g_pWordLattice->m_refAmScore + LANGUAGE_WEIGHT * g_pWordLattice->m_refLmScore);

	g_pAM->ResetObserveBuffer(pVector);//将pVector传给pHMM另一方面,给g_pHMM进行初始化

	//下面在节点债中进行遍历，进行模型内的扩展。
for(int beamLevel = 0; beamLevel < 2; beamLevel++)
{
	keyCt = g_pPath->keyHead[g_pPath->flag1][beamLevel];
	//
	while(keyCt >= 0)
//	for(keyCt = 0; keyCt < MODULE_VALUE1 + MODULE_VALUE2; keyCt++)
	{
		nextKeyKeep = g_pPath->keyList[g_pPath->flag1][keyCt].nextKey;

		CurIndex = g_pPath->pBucketHead[g_pPath->flag1][keyCt];
		//在pBucket[flag1][keyCt]中查看有无节点，如果有则取出进行扩展
		preIndex = -1;			
		while(CurIndex >= 0)
		{
			SeedNode = g_pPath->pTokenPath[CurIndex];

			SeedNode.amScore -= g_pWordLattice->m_refAmScore;
			SeedNode.lmScore -= g_pWordLattice->m_refLmScore;
			
			g_pAM->ExpandNode(SeedNode);//对SeedNode进行模型内的扩展
			
			g_pPath->ReleaseBlock(g_pPath->flag1, keyCt, preIndex, CurIndex, beamLevel);
			//在pathToken寨中释放掉刚刚进行扩展的种子节点
			
			preIndex = CurIndex;
			CurIndex = g_pPath->pTokenPath[CurIndex].nextToken;
			//获得下一个节点
		}
		//
		keyCt = nextKeyKeep;
	}	//注意在这个扩展过程中也可能有nNodes为零的 情况
}

	//printf("%d\n",t);
	//在输出词后，如果不减已经输出的词的概率，则下面两行代码有效；否则，请注掉。
	//
	g_pWordLattice->m_refAmScore = 0;
	g_pWordLattice->m_refLmScore = 0;
	//
	g_pPath->BeamPruning(0, MODULE_VALUE1, INTRA_WORD_THRESHOLD, M_EPS_INTRA);
	//进行模型内的裁减
	g_pPath->m_lProbMax = NEG_HUGE;//-= 300000L;		
	//
	
//	g_pLM->ResetBuffer( ); //语言模型的Cache的重新设置, By 高升注掉
	g_pAM->ResetAmCacheBuffer(); //Nov.3,2000

	//下面进行模型间的词内扩展
	//与其他扩展一样，他也是在节点寨中进行遍历，找到可以扩展的节点进行扩展。
	keyCt = g_pPath->keyHead[g_pPath->flag2][0];
	while(keyCt >= 0)
	//for(keyCt = 0; keyCt < MODULE_VALUE1; keyCt++)
	{
		nextKeyKeep = g_pPath->keyList[g_pPath->flag2][keyCt].nextKey;
		//
		CurIndex = g_pPath->pBucketHead[g_pPath->flag2][keyCt];
		preIndex = -1;
		
		while(CurIndex >= 0){				
			SeedNode = g_pPath->pTokenPath[CurIndex];			

			int state = g_pAM->GetHMMMaxState(SeedNode.modelId) - 1;
			int i_s = SeedNode.tokenActive;			

			if (i_s == state){

				g_pLextree->IntraWordExtend(SeedNode, timeAxis);
				//
				g_pPath->ReleaseBlock(g_pPath->flag2, keyCt, preIndex, CurIndex, 0);
				//释放种子节点								
			}
			else
				preIndex = CurIndex;

			CurIndex = g_pPath->pTokenPath[CurIndex].nextToken;
		}
		keyCt = nextKeyKeep;
	}

	if(g_pModeManager->GetActiveMode() == DICTATION_MODE)	
		BOOL fResult = g_pLextree->InterWordExtend(timeAxis);

	g_pPath->BeamPruning(MODULE_VALUE1, MODULE_VALUE1 + MODULE_VALUE2, INTER_WORD_THRESHOLD, M_EPS_INTER);
	//进行模型间的裁剪

	g_pPath->m_lProbMax = NEG_HUGE;//+= 70000L;

	LATTICENODE pWordList[MAX_WORD_CANDIDATES];

	num = g_pLextree->GetWordNumber( );  //获得Lextree里WordLattice的词对个数；
	if(num > 0){
		//如果g_pLextree中的词对个数大于零，则将其得到他并将其压入 g_pWordLattice的词对栈 中
		g_pLextree->GetWordLatticeList(pWordList); 
		g_pWordLattice->PushLattice(timeAxis, num, pWordList);		
	}

	g_pPath->ExchangeStack( );
	//交换flag1和flag2一、并对pBucketHead进行复位
		
	timeAxis++;

	int terminalTrace = fifoNull || sentenceEnd;

#ifdef INCREMENTAL_SEARCH

if(g_pModeManager->GetActiveMode() == DICTATION_MODE)
{
	if(timeAxis >= MIN_TRACE_START_TIME && !terminalTrace)
	{
		if(timeAxis - g_pWordLattice->m_traceStartTime >= MAX_TRACE_TIME_SLOT)
		{
			if(timeTmp = g_pWordLattice->PartialTraceBack(g_pWordLattice->m_traceStartTime))
				g_pWordLattice->m_traceStartTime = timeTmp;
		}
		if(g_pWordLattice->m_traceCount == MAX_CONFIRM_BLOCK)
		{
			if(g_pWordLattice->GeneratePartialWord())
			{
				GetPartialFirmwords();
//				SendMessage(g_hMsgTarget, WM_DRAGONVOICE_FIRM, 0, 1);
				printf("%s",m_firmWords);
			}
		}
	}
}

	if(timeAxis < MAX_CONTINUOUS_TIME)
	{
		if(sentenceEnd)//terminalTrace)
		{
			GetLastFirmwords();
//			SendMessage(g_hMsgTarget, WM_DRAGONVOICE_FIRM, 0, 0);
			printf("%s",m_firmWords);
			OnepassSearchEngineInit();
		}
	}
	else
	{
		GetLastFirmwords();
//		SendMessage(g_hMsgTarget, WM_DRAGONVOICE_FIRM, 0, 0);
		printf("%s",m_firmWords);
		OnepassSearchEngineInit();
	}
#else
/*	if(timeAxis >= MIN_TRACE_START_TIME && !terminalTrace)
	{
		if(timeAxis - g_pWordLattice->m_traceStartTime >= MAX_TRACE_TIME_SLOT)
		{
			if(timeTmp = g_pWordLattice->PartialTraceBack(g_pWordLattice->m_traceStartTime))
				g_pWordLattice->m_traceStartTime = timeTmp;
		}
		if(g_pWordLattice->m_traceCount == MAX_CONFIRM_BLOCK)
		{
			if(g_pWordLattice->GeneratePartialWord())
			{
				SendMessage(g_hMsgTarget, WM_DRAGONVOICE_FIRM, 0, 1);
			}
		}
	}
*/
	if(terminalTrace)
	{
		GetLastFirmwords();
//		SendMessage(g_hMsgTarget, WM_DRAGONVOICE_FIRM, 0, 0);
		printf("%s",m_firmWords);
		OnepassSearchEngineInit();
	}
/*	else
	{
		SendMessage(g_hMsgTarget, WM_DRAGONVOICE_FIRM, 0, 0);
		OnepassSearchEngineInit();
	}
	*/
#endif

}




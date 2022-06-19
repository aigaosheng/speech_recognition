// Path.cpp: implementation of the CPath class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "const.h"
#include "global.h"

#include "path.h"

//
//#define DEBUG_HASH_EFFICIENT
#ifdef  DEBUG_HASH_EFFICIENT
int intraFound = 0, intraTotal = 0, intraCt = 0;
int interFound = 0, interTotal = 0, interCt = 0;
int	wordFound = 0, wordTotal = 0, wordCt = 0;
int maxdepth[3]={0,0,0};
#endif
//

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPath::CPath()
{
	//flag1 = 0;                
	//flag2 = 1;                
	//freeBlockTop = 0;	
	memset(pTokenPath, 0, sizeof(PHONENODE) * NSLMAX);
}

CPath::~CPath()
{

}

void CPath::Init( )
{
	int j;

	//for(j = 0; j < (MODULE_VALUE1 + MODULE_VALUE2); j++)
	//	pBucketHead[0][j] = pBucketHead[1][j] = -1;
	memset(pBucketHead, -1, sizeof(int) * (MODULE_VALUE1 + MODULE_VALUE2) * 2);
	memset(keyHead, -1, sizeof(int) * 2 * 2);
	memset(keyTail, -1, sizeof(int) * 2 * 2);
	for(j = 0; j < MODULE_VALUE1 + MODULE_VALUE2; j++)
		keyList[0][j].preKey = keyList[0][j].nextKey = keyList[1][j].preKey = keyList[1][j].nextKey = -1;

	for(j = 0; j < NSLMAX; j++)
		freeBlockIdx[j] = j;	
	//memset(freeBlockIdx, -1, sizeof(int) * NSLMAX);
	
	flag1 = 0;                

	flag2 = 1;    
	
	freeBlockTop = 0;

	m_lProbMax = NEG_HUGE;	

	m_nHisWord1 = HEAD_WORD; //高升编辑
	m_nHisWord2= HEAD_WORD;//高升编辑

	return;
}



void CPath::PushIntraHMMNode(int nNodes, LPPHONENODE pNodes )
{
	int i;
	
	for(i = 0; i < nNodes; i++)
		InsertPath(flag2, pNodes[i], 0);	

	return;
}

	


void CPath::PushInterWordNode(int nNodes, LPPHONENODE pNodes)
{
	int i;

	for(i = 0; i < nNodes; i++)
		InsertPath(flag2, pNodes[i], 2);

	return;
}
	


void CPath::ExchangeStack( )
{
	int temp;
	int i;

	temp = flag1;
	flag1 = flag2;
	flag2 = temp;
	/*
	ASSERT(keyHead[flag2][0] == -1);
	ASSERT(keyHead[flag2][1] == -1);
	ASSERT(keyTail[flag2][0] == -1);
	ASSERT(keyTail[flag2][1] == -1);

	keyHead[flag2][0] = keyHead[flag2][1] = -1;
	keyTail[flag2][0] = keyTail[flag2][1] = -1;
	for(i = 0; i < MODULE_VALUE1 + MODULE_VALUE2; i++)
	{
		ASSERT(pBucketHead[flag2][i] == -1);
		ASSERT(keyList[flag2][i].preKey == -1);
		ASSERT(keyList[flag2][i].nextKey == -1);

		pBucketHead[flag2][i] = -1;
		keyList[flag2][i].preKey = keyList[flag2][i].nextKey = -1;
	}

*/
	return;
}



void CPath::ReleaseBlock(int flag, int keyCt, int preIndex, int curIndex, int beamLevel)
{
	freeBlockTop--;
	ASSERT(freeBlockTop >= 0);
	freeBlockIdx[freeBlockTop] = curIndex;

	if(pBucketHead[flag][keyCt] == curIndex)
	{
		pBucketHead[flag][keyCt] = pTokenPath[curIndex].nextToken;
		//如果某个键值的路径为空，则从键值双向链表中删除此键值节点
		if(pBucketHead[flag][keyCt] == -1)
			RemoveBidirectKeylist(flag, beamLevel, keyCt);
	}
	else
	{
		ASSERT(preIndex >= 0);
		pTokenPath[preIndex].nextToken = pTokenPath[curIndex].nextToken;
	}
	return;
}

inline void CPath::InsertBidirectKeylist(int stackIdx, int beamLevel, int key)
{
	int tailkey;

	tailkey = keyTail[stackIdx][beamLevel];

	if(keyHead[stackIdx][beamLevel] == -1)
	{
		keyHead[stackIdx][beamLevel] = key;
		keyTail[stackIdx][beamLevel] = key;
		keyList[stackIdx][key].preKey = -1;
		keyList[stackIdx][key].nextKey = -1;
	}
	else
	{
		ASSERT(keyTail[stackIdx][beamLevel] > -1);
		keyList[stackIdx][key].preKey = tailkey;
		keyList[stackIdx][key].nextKey = -1;
		keyList[stackIdx][tailkey].nextKey = key;
	}
	//修改尾节点
	keyTail[stackIdx][beamLevel] = key;
}

inline void CPath::RemoveBidirectKeylist(int flag, int beamLevel, int keyCt)
{
	int prekey, nextkey;

	if(keyCt == keyHead[flag][beamLevel])
	{
		//如果是首节点，但不是尾节点
		if(keyCt != keyTail[flag][beamLevel])
		{
			nextkey = keyList[flag][keyCt].nextKey;
			keyHead[flag][beamLevel] = nextkey;
			keyList[flag][nextkey].preKey = -1;
		}
		//如果是首节点，又是尾节点，则键值表变空
		else
		{
			keyHead[flag][beamLevel] = keyList[flag][keyCt].nextKey;
			ASSERT(keyHead[flag][beamLevel] == -1);
			keyTail[flag][beamLevel] = -1;
		}
	}
	else
	{
		//如果不是尾节点
		if(keyCt != keyTail[flag][beamLevel])
		{
			prekey = keyList[flag][keyCt].preKey;
			nextkey = keyList[flag][keyCt].nextKey;
			keyList[flag][prekey].nextKey = nextkey;
			keyList[flag][nextkey].preKey = prekey;
		}
		//如果是尾节点，则键值表变空
		else
		{
			prekey = keyList[flag][keyCt].preKey;
			nextkey = keyList[flag][keyCt].nextKey;
			ASSERT(nextkey == -1);
			ASSERT(prekey >= 0);
			keyList[flag][prekey].nextKey = nextkey;
			//更新尾节点
			keyTail[flag][beamLevel] = prekey;
		}
	}
	//复位被删除的节点
	keyList[flag][keyCt].nextKey = -1;
	keyList[flag][keyCt].preKey = -1;
}


void CPath::BeamPruning(int startNo, int moduleValue, int pathThreshold, const long * M_EPS)
{
	long probMax;
	long probTmp;
	int  curIndex, preIndex, keyCt, beamLevel, curkey, nextKeyKeep;
	int iii;
	short codeNum[BLOCKNUM + 1];
	
	
/*
	probMax = neg_huge;
	for(keyCt = startNo; keyCt < moduleValue; keyCt++)
	{
		curIndex = pBucketHead[flag2][keyCt];
		while(curIndex >= 0)
		{
			probTmp = ACOUSTIC_WEIGHT * pTokenPath[curIndex].amScore + LANGUAGE_WEIGHT * pTokenPath[curIndex].lmScore;
			if(probTmp > probMax)
				probMax = probTmp;
			curIndex = pTokenPath[curIndex].nextToken;
		}
	}
*/
/*#ifdef  DEBUG_HASH_EFFICIENT
	int nullct = 0, cout=0;
	for(keyCt = startNo; keyCt < moduleValue; keyCt++)
	{
		curIndex = pBucketHead[flag2][keyCt];
		cout=0;
		while(curIndex >= 0)
		{
			cout++;
			curIndex = pTokenPath[curIndex].nextToken;
		}
		if(!cout)
			nullct++;
	}
	if(startNo)
		printf("%7.3f", nullct*100.0/(moduleValue-startNo));
	else
		printf("\n%7.3f", nullct*100.0/(moduleValue-startNo));
#endif
*/
	for(iii = 0; iii <= BLOCKNUM; iii++)
		codeNum[iii] = 0;

	//if(startNo)
	//	probMax = m_lProbMax + 70000L;
	//else
		probMax = m_lProbMax;

	if(startNo)
		beamLevel = 1;
	else
		beamLevel = 0;

	curkey = keyHead[flag2][beamLevel];

	while(curkey >= 0)
//	for(keyCt = startNo; keyCt < moduleValue; keyCt++)
	{
		curIndex = pBucketHead[flag2][curkey];
		ASSERT(curIndex >= 0);
		preIndex = -1;
		while(curIndex >= 0)
		{
//			probTmp = ACOUSTIC_WEIGHT * (pTokenPath[curIndex].amScore + pTokenPath[curIndex].deltaScore) + LANGUAGE_WEIGHT * pTokenPath[curIndex].lmScore;
			probTmp = ACOUSTIC_WEIGHT * pTokenPath[curIndex].amScore + LANGUAGE_WEIGHT * pTokenPath[curIndex].lmScore;
			/*
			for(iii = 0; iii < BLOCKNUM; iii++)
			{
				if(probTmp >= (probMax - M_EPS[iii]))
				{
					pTokenPath[curIndex].probLevel = iii;
					codeNum[iii]++;
					break;
				}
			}
			if(iii == BLOCKNUM)
			{
				pTokenPath[curIndex].probLevel = BLOCKNUM;
				codeNum[BLOCKNUM]++;
			}
			/*/
			if(probTmp < (probMax - M_EPS[BLOCKNUM-1]))
			{
				pTokenPath[curIndex].probLevel = BLOCKNUM;
				codeNum[BLOCKNUM]++;
			}
			else
			{
				for(iii = 0; iii < BLOCKNUM; iii++)
				{
					if(probTmp >= (probMax - M_EPS[iii]))
					{
						pTokenPath[curIndex].probLevel = iii;
						codeNum[iii]++;
						break;
					}
				}
				ASSERT(iii != BLOCKNUM);
			}
			//
			preIndex = curIndex;
			curIndex = pTokenPath[curIndex].nextToken;
		}
		curkey = keyList[flag2][curkey].nextKey;
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

	//totalNum=0;
	curkey = keyHead[flag2][beamLevel];

	while(curkey >= 0)
//	for(keyCt = startNo; keyCt < moduleValue; keyCt++)
	{
		curIndex = pBucketHead[flag2][curkey];
		preIndex = -1;
		//
		nextKeyKeep = keyList[flag2][curkey].nextKey;
		while(curIndex >= 0)
		{
			if(pTokenPath[curIndex].probLevel <= code)
			{
				//set this path activated
	//			totalNum++;
				preIndex = curIndex;
			}
			else
			{
				//release this block
				ReleaseBlock(flag2, curkey, preIndex, curIndex, beamLevel);
				preIndex = preIndex;
			}
			curIndex = pTokenPath[curIndex].nextToken;
		}
		//
		//curkey = keyList[flag2][curkey].nextKey;
		curkey = nextKeyKeep;
	}
/*	
	FILE *fdbg=fopen("debugA.txt", "a+t");
	fprintf(fdbg, "%-6d ", totalNum);
	totalNum=0;
	for(iii = 0; iii <= BLOCKNUM; iii++)
		totalNum+=codeNum[iii];
	fprintf(fdbg, "%-6d\n", totalNum);

	fclose(fdbg);
*/
}

void CPath::PushInitHMMNode(int nNodes, LPPHONENODE pNodes)
{
	int i;

	for(i = 0; i < nNodes; i++)
		InsertPath(flag1, pNodes[i], 0);

	return;
}


void CPath::PushInterHMMNode(int nNodes, LPPHONENODE pNodes)
{
	int i;
	
	for(i = 0; i < nNodes; i++)
		InsertPath(flag2, pNodes[i], 1);
	
	return;
}


int CPath::InsertPath(int stackIdx, PHONENODE path, int type)
{

	int curIndex, preIndex, bucketSign, module, startNo, moduleValue;
	int preIndexTmp, curIndexTmp, minKey, minIndex, minPreIndex, keyCt;
	int languageWeight, beamLevel;
	__int64  keyValue;
	//int  keyValue;
	long probMin, probTmp, probSum;

	switch(type)
	{
		case 0:
			languageWeight = LANGUAGE_WEIGHT;
			bucketSign = 0;
			module = MODULE_VALUE1;
			startNo = 0; 
			moduleValue = MODULE_VALUE1;
			//keyValue = (path.historyWord1 + path.historyWord2 + path.grammarNode * path.modelId + path.tokenActive) % module;
			//
			beamLevel = 0;
			break;
		case 1:
			//inter word or inter-model extention
			languageWeight = LANGUAGE_WEIGHT;
			bucketSign = MODULE_VALUE1;
			module = MODULE_VALUE2;
			startNo = MODULE_VALUE1; 
			moduleValue = MODULE_VALUE1 + MODULE_VALUE2;
	//		keyValue = (path.historyWord1 + path.historyWord2 + path.grammarNode * path.modelId + path.tokenActive) % module;
			//
			beamLevel = 1;
			
			break;
		case 2:
			//inter word or inter-model extention
			languageWeight = LANGUAGE_WEIGHT;
			bucketSign = MODULE_VALUE1;
			module = MODULE_VALUE2;
			startNo = MODULE_VALUE1; 
			moduleValue = MODULE_VALUE1 + MODULE_VALUE2;
	//		keyValue = (path.historyWord1 + path.historyWord2 + path.grammarNode * path.modelId + path.tokenActive) % module;
			beamLevel = 1;
			break;
		default:
			return 0;
	}
	//
//	path.flag = type;
	//

#ifdef DEBUG_HASH_EFFICIENT
	switch(type)
	{
	case 0:
		intraTotal++;
		break;
	case 1:
		interTotal++;
		break;
	case 2:
		wordTotal++;
		break;
	default:
		return 0;
	}
	int depth=0;
	
#endif

	probSum = ACOUSTIC_WEIGHT * path.amScore + languageWeight * path.lmScore;
	//printf("%d\n", maxoffset);

	keyValue = (path.historyWord1 + path.historyWord2 + path.grammarNode * path.modelId + path.tokenActive) % module; //best Hash, minimal depth
	//keyValue = (path.grammarNode * path.modelId + path.tokenActive) % module; //maximal depth
	//keyValue = (path.historyWord1 + path.historyWord2 + path.grammarNode + path.modelId +path.tokenActive)% module; //

	curIndex = pBucketHead[stackIdx][keyValue+bucketSign];
	preIndex = -1;

	while(curIndex >= 0)
	{
#ifdef DEBUG_HASH_EFFICIENT
	depth++;
	switch(type)
	{
	case 0:
		intraCt++;
		break;
	case 1:
		interCt++;
		break;
	case 2:
		wordCt++;
		break;
	default:
		return 0;
	}

#endif
		if(path.historyWord1 == pTokenPath[curIndex].historyWord1
		  && path.historyWord2 == pTokenPath[curIndex].historyWord2
		  && path.grammarNode == pTokenPath[curIndex].grammarNode
		  && path.modelId == pTokenPath[curIndex].modelId
		  && path.tokenActive == pTokenPath[curIndex].tokenActive)
		{
			break;
		}

		//
		preIndex = curIndex;
		curIndex = pTokenPath[curIndex].nextToken;
	}

#ifdef DEBUG_HASH_EFFICIENT
	switch(type)
	{
	case 0:
		if(depth>maxdepth[0])
			maxdepth[0]=depth;
		break;
	case 1:
		if(depth>maxdepth[1])
			maxdepth[1]=depth;
		break;
	case 2:
		if(depth>maxdepth[2])
			maxdepth[2]=depth;
		break;
	default:
		return 0;
	}
#endif

	if(curIndex >= 0)
	{
#ifdef DEBUG_HASH_EFFICIENT
	switch(type)
	{
	case 0:
		intraFound++;
		break;
	case 1:
		interFound++;
		break;
	case 2:
		wordFound++;
		break;
	default:
		return 0;
	}

#endif

		if(probSum > ACOUSTIC_WEIGHT * pTokenPath[curIndex].amScore + languageWeight * pTokenPath[curIndex].lmScore)
		{
			path.nextToken = pTokenPath[curIndex].nextToken;
			pTokenPath[curIndex] = path;	
			//计算最大概率
			if(m_lProbMax < probSum)
				m_lProbMax = probSum;
		}
	}
	else
	{ //this path is not in stack
		//apply a free block from the stack
		if(freeBlockTop >= NSLMAX)
		{
			//find the path with minimal score and replace it with the current if current score is more than the minimal path
			probMin = POS_HUGE;
			minIndex = -1;

			keyCt = keyHead[stackIdx][beamLevel];
			while(keyCt >= 0)
			//for(keyCt = startNo; keyCt < moduleValue; keyCt++)
			{
				int nextKeyKeep = keyList[stackIdx][keyCt].nextKey;

				curIndexTmp = pBucketHead[stackIdx][keyCt];
				preIndexTmp = -1;
				while(curIndexTmp >= 0)
				{
					probTmp = ACOUSTIC_WEIGHT * pTokenPath[curIndexTmp].amScore + LANGUAGE_WEIGHT * pTokenPath[curIndexTmp].lmScore;
					if(probTmp < probMin)
					{
						probMin = probTmp;
						minIndex = curIndexTmp;
						minKey = keyCt;
						minPreIndex = preIndexTmp;
					}
					preIndexTmp = curIndexTmp;
					curIndexTmp = pTokenPath[curIndexTmp].nextToken;
				}
				//
				keyCt = nextKeyKeep;
			}
			ASSERT(minIndex >= 0);
			if(probSum > probMin)
				ReleaseBlock(stackIdx, minKey, minPreIndex, minIndex, beamLevel);
			else
				return 1;
		}
		ASSERT(freeBlockTop < NSLMAX);
		ASSERT(freeBlockIdx[freeBlockTop] != -1);
		if(pBucketHead[stackIdx][keyValue+bucketSign] != curIndex)
		{
			//the hash table is not empty and modify the last item and make nextToken point to new path
			ASSERT(pTokenPath[preIndex].nextToken == -1);
			pTokenPath[preIndex].nextToken = freeBlockIdx[freeBlockTop];
		}
		else
		{
			ASSERT(pBucketHead[stackIdx][keyValue+bucketSign] == -1);
			pBucketHead[stackIdx][keyValue+bucketSign] = freeBlockIdx[freeBlockTop];
			//如果该键值链表初创，则在键值双向表中创建节点
			InsertBidirectKeylist(stackIdx, beamLevel, keyValue + bucketSign);
		}
		//
		pTokenPath[freeBlockIdx[freeBlockTop]] = path;
		freeBlockIdx[freeBlockTop] = -1;
		freeBlockTop++;
		ASSERT(freeBlockTop <= NSLMAX);
		//
		if(m_lProbMax < probSum)
			m_lProbMax = probSum;
	}
	return 1;

}

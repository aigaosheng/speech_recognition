//
//定义数据结构
//
#ifndef __STRUCT_H
#define	__STRUCT_H

#include	"const.h"

#define USE_SSE

/////////////////////////////////////////////////////////////////////////////////////////
// define by menglei

#define MAX_MERGE_NUM 115//最大合并数
typedef struct tag_LatticeNode{
	BYTE	toneId;
	BYTE	leftTone;
	WORD	wordId;
	WORD	hisWord1, hisWord2;//preWordPos;
	short	modelDur;
	short	leftTriphone;
	short	preTime;
	short	preWordPos;
	short	lastModel;
	long	amScore, lmScore;
} LATTICENODE, *LPLATTICENODE;

typedef struct tag_LexicalTree{
	char toneId;
	BYTE linkNum; //the number of nodes connected with this node. if<0, leaf node and abs
				   //(linkNum)=the number of homosynom
	short nodeId; //the phone ID or syllable ID which corresponds to this node.
	long startNode; //the first child node connected with this node
	WORD wordFirst; //the code of first word in the range
	WORD wordLast; //the code of last word in the range
	long maxUnigramProb; //maximal unigram in words sharing path from root to this node
}LEXTREE, *LPLEXTREE;

////////////////////////////////////////////////////////////////////////////////////////
// define by kewen

typedef struct tag_PhoneNode{
	BYTE	tokenActive;		//store HMM state and active mark. The higest 7 bits 
								//represents HMM state and lowest bit acrive mark,'1':active;'0':deactive
	BYTE	leftTone;			//high 4bits: left tone, low 4 bits: self tone
	DWORD	grammarNode;		//grammar node in lexical tree
	short	leftTriphone;		//record left context 
	short	modelId;
	WORD	historyWord1, historyWord2;
	short	wordDuration;		//word duration. Reset it to zero when new word occurs. For debug
	short	startTime;			//word start time. update it  when new word occurs
	short	preWordPos;
	long	amScore,			//partial path AM score
			lmScore,			//partial path LM score
			lmLookaheadScore;	//lookahead Lm score of current grammar node

	int   probLevel;
	int		nextToken;
	//int leftChild, rightChild;
	//
	//short	flag;
//	long deltaScore;
} PHONENODE, *LPPHONENODE;

typedef struct tag_Toplog
{
// Toplogy
	int No_State;
	int No_Out;
	int No_Trans;		
	int *nj;			// nj[i_nState]. store jump branch number in a state.BY GS
	int *indj;			// indj[i_nState*i_nState]; store jump destination state. BY GS
	int *aLinks;		// i_aLink[i_nState][i_nState]; store No of jump branches for all
						// branches in one HMM to index transmittion probability A.
	int *bLinks;		// i_bTrans[i_nState][i_nState];store No of distribution branches for all
						// branches in one HMM to index B parameters of HMM
	int	*OutputID;
	float *A;				// A[No_Trans]
} HMM, *LPHMM;

/*
typedef struct tag_Toplog2
{
	// Data for Training Count
	// Data for Model
	float CoeMix[NO_MIX];					// Means[No_out][No_Mix][No_Dim]
	float Means[NO_MIX * NO_DIM];			// Variance[No_out][No_Mix][No_Dim];
	float Variance[NO_MIX * NO_DIM];		// CoeMix[No_out][No_Mix];
} OUTPUT, *LPOUTPUT;
*/

typedef struct tag_Toplog2_1
{
	float CoeMix[NO_MIX];
#ifdef USE_SSE
	__m128 Means[NO_MIX][NO_DIM_ALIGNED];
	__m128 Variance[NO_MIX][NO_DIM_ALIGNED];
#else
	float Means[NO_MIX][NO_DIM_ALIGNED*4];
	float Variance[NO_MIX][NO_DIM_ALIGNED*4];
#endif
} OUTPUT_ALIGNED, *LPOUTPUT_ALIGNED;

typedef struct tag_Bigram
{
	WORD	W2;
	long	lProb;
	long	lBackoff;
} BIGRAM, *LPBIGRAM;

typedef struct tag_LMBufferNode
{
	long	LMScore;
	int		nGrammarId;
	//WORD	maxWord;
} LM_BUFFER_NODE, *PLM_BUFFER_NODE;

typedef struct tag_LMBufferHead
{
	DWORD			w1w2;
	PLM_BUFFER_NODE pNode;
	WORD			szAlloced;
	WORD			nNode;
} LM_BUFFER_HEAD, *PLM_BUFFER_HEAD;

typedef struct
{
	int	preKey;
	int	nextKey;
}BIDIRECT_KEY_LIST;
#endif
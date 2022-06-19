/*****************************************************************
Program: struct.h
Description: define some variable structure
Written By: gao sheng
Date: Oct.28, 1999
Date Modified: April 3,2000
Note: modified because pitch information is considered in the feature
******************************************************************/
#ifndef __STRUCT_H
#define	__STRUCT_H

#include "include\const.h"

typedef	struct
{
	char	pinYin[12];
	int		shMuIndex;
	int		yMuIndex;
}SYLLABLE_CONSTRUCT_TYPE;

//define struct for token,i.e. a node in HMM state-time network.
typedef struct{
	unsigned char	tokenActive; //store HMM state and active mark. The higest 7 bits represents HMM state and lowest bit acrive mark,'1':active;'0':deactive
	unsigned char	leftTone;  //high 4bits: left tone, low 4 bits: self tone
	short			leftTriphone; //record left context
	short			modelId;
	unsigned short	historyWord1, historyWord2;
	short			wordDuration; //word duration. Reset it to zero when new word occurs. For debug
	short			startTime; //word start time. update it  when new word occurs
	short			preWordPos;
	int				tokenPruneLevel;
	unsigned int	grammarNode; //grammar node in lexical tree
	long			amScore, //partial path AM score
					lmScore, //partial path LM score
					lmLookaheadScore; //lookahead Lm score of current grammar node
	int				nextToken;
	//int leftChild, rightChild;
}CPhoneNode;

typedef struct{
	unsigned char	toneId;
	unsigned char	leftTone;
	unsigned short	wordId;
	unsigned short	hisWord1, hisWord2;//preWordPos;
	short modelDur;
	short leftTriphone;
	short	preTime;
	short	preWordPos;
	short	lastModel;
	long	amScore, lmScore;
}CWordLattice;

typedef struct{
	unsigned short bigramNumber; //bigram number of current word
	long bigramPos; //start position of the first bigram of current word
}CBigramIdx;

typedef struct{
	unsigned short word; //word code
	long	probBigram,
			backoffWeight; //the weight of Bigram when backoff trigram to bigram
}CBigram;

typedef struct{
	unsigned short trigramNumber; //bigram number of current word
	long tigramPos; //start position of the first bigram of current word
}CTigramIdx;

typedef	struct{
	unsigned short	word1, word2;
	long	probTrigram,
			backoffWeight;
}CTrigram;

typedef struct{
	char	word[2*MAX_WORD_LENGTH+1];
	long	probUnigram,
			backoffWeight;
}CUnigram;

//define node structure of lexical tree
typedef struct{
	char toneId;
	short nodeId; //the phone ID or syllable ID which corresponds to this node.
	short linkNum; //the number of nodes connected with this node. if<0, leaf node and abs(linkNum)=the number of homosynom
	long startNode; //the first child node connected with this node
	unsigned short wordFirst; //the code of first word in the range
	unsigned short wordLast; //the code of last word in the range
	long maxUnigramProb; //maximal unigram in words sharing path from root to this node
}CLexicalTree;

//define bucket structure 
typedef struct{
	unsigned short	w1, w2;
	long prob;
}CDisLmCache;

// Hidden Markov Model Definition
typedef	struct
{
// Toplogy
	int No_State;
	int No_Out;
	int No_Trans;		
	int *nj;			// nj[i_nState]. store jump branch number in a state.BY GS
	int *indj;			// indj[i_nState*i_nState]; store jump destination state. BY GS
	int *aLinks;		// i_aLink[i_nState][i_nState]; store No of jump branches for all branches in one HMM
						// to index transmittion probability A.
	int *bLinks;		// i_bTrans[i_nState][i_nState];store No of distribution branches for all branches in one HMM
						// to index B parameters of HMM
	int	*OutputID;
	//float *A;				// A[No_Trans]
	long *A;				// A[No_Trans]
} CHmm;
typedef	struct
{
	// Data for Training Count
	// Data for Model
	float *Means;			// Means[No_out][No_Mix][No_Dim]
	float *Variance;		// Variance[No_out][No_Mix][No_Dim];
	float *CoeMix;			// CoeMix[No_out][No_Mix];

} COutput;
#endif
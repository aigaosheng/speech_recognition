// Const.h: interface for the CConst class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONST_H__EF9C4F9E_DD32_4053_BFFE_F4A79D3AA525__INCLUDED_)
#define AFX_CONST_H__EF9C4F9E_DD32_4053_BFFE_F4A79D3AA525__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////////////////
// 从原来的const.h中复制过来的

#define		NO_DIM	42//39//	//特征向量的维数
#define		NO_DIM_ALIGNED	44
#define		NO_MIX	16//2//8//4
#define		FEATURE_DIM	14//13//13	//于是数据的维数

#define LOG_BASE 1.0001       	 // For viterbi log scaling
#define NEG_HUGE -1000000000L
#define POS_HUGE  1000000000L

#define MAX_STATE				6
#define MAX_OUT					4
#define MAX_TRANS				9
#define	MAX_NODE_TRI			14000//6000//4000//6000
#define	TOTAL_TRIPHONES			13307//8327//13470//11973//10801//5342//4901//4734//4542//303//5442//2648
#define	TRI_SILENCE_CODE		TOTAL_TRIPHONES-1
#define	TRI_NO_HMM				13307//8327//13470//11973//10801//5342//4901//4734//4542//303//5442//2648
//#define	NO_OUTPUT				1090//2733//2872//2461//2704//2591//3063//2684//2581//2646//974//3213//1647


#define MAX_CODE_INITIAL_PART	23//19
#define	MAX_CODE_FINAL_PART		60//72
#define	TOTAL_SYLLABLES		409//408//668
#define TOTAL_INITIAL		24	//ke wen
#define TOTAL_FINAL			37	//ke wen
#define	TOTAL_PHONES		62//74
#define	SILENCE_CODE		61//73

#define	MAX_NUM_PHONE		200
#define	MAX_NUM_WORD		60
#define	MAX_SENTENCE_LENGTH	60


#define		HEAD_WORD	13625
#define		TAIL_WORD	13625
//#define		HEAD_WORD	13775//13575//13554L//13555L//13558L//
//#define		TAIL_WORD	13775//13575//13554L//13555L//13558L//

#define		ACOUSTIC_WEIGHT	1//0
#define		LANGUAGE_WEIGHT	10//15//

#define		MAX_WORD_CANDIDATES	 20//50//100//
//
//define pitch constant
#define	TOTAL_PITCH_PHONES		306
//#define	TOTAL_PITCH_PHONES		210//394//
#define PITCH_SILENCE_CODE	TOTAL_PITCH_PHONES-1

#define	PITCH_0		0 //轻声
#define	PITCH_1		1 //1声
#define	PITCH_2		2 //2声
#define	PITCH_3		3  //3声
#define	PITCH_4		4 //4声
#define	PITCH_SIL0	5 //轻声
#define	PITCH_SIL1	6 //1声
#define	PITCH_SIL2	7 //2声
#define	PITCH_SIL3	8 //3声
#define	PITCH_SIL4	9 //4声
#define	PITCH_SIL	10 //没有声调,即不考虑声调
#define	PITCH_NULL	5 //不考虑声调
//for predict-LM cache
#define MAX_CACHE_LIST	10
#define MAX_TREE_NODE	120000
/////////////////////////////////////////////////////////////////////////////////////////
// define by menglei

#define MAX_MERGE_NUM 1000//最大合并数
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
	int		nextToken;
	//int leftChild, rightChild;
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

typedef struct tag_Toplog2
{
	// Data for Training Count
	// Data for Model
	float CoeMix[NO_MIX];					// Means[No_out][No_Mix][No_Dim]
	float Means[NO_MIX * NO_DIM];			// Variance[No_out][No_Mix][No_Dim];
	float Variance[NO_MIX * NO_DIM];		// CoeMix[No_out][No_Mix];
} OUTPUT, *LPOUTPUT;

typedef struct tag_Toplog2_1
{
	float CoeMix[NO_MIX];
	float Means[NO_MIX][NO_DIM_ALIGNED];
	float Variance[NO_MIX][NO_DIM_ALIGNED];
} OUTPUT_ALIGNED, *LPOUTPUT_ALIGNED;

typedef struct tag_Bigram
{
	WORD	W2;
	long	lProb;
	long	lBackoff;
} BIGRAM, *LPBIGRAM;

/////////////////////////////////////////////////////////////////////////////////////////
// define by wangshengjun
#define		NO_WORDS	39969//词典中词的个数


// Constants for Feature
#define 	PCEP		12
#define 	PEDE		3

#define		MAX_SPEECH_LENGTH  1200//800
#define		MAX_WORD_LENGTH    7	//每个词的最大长度

//
#define		FFTLEN		512
#define		FILTNUM		25
#define		FS			16.0
#define		PI			3.1415926536


// Constants for PreProcessing
#define 	PRE			0.97
#define 	HFRAMELEN	192
#define 	FRAMELEN	384


#define		NSLMAX 60000//120000//16000//              // The depth of stack

//for path hash
#define		PENALTY 0//-10000//
#define		MODULE_VALUE1	809
#define		MODULE_VALUE2	3001
#define		MAX_BUCKET_LIST	1000

#define		bbase 10

#define		INTRA_WORD_THRESHOLD	1000//1200//2000//800//
#define		INTER_WORD_THRESHOLD	800//3000//1000//500//

#define		BLOCKNUM    8            // Multithreshold Pruning

#endif // !defined(AFX_CONST_H__EF9C4F9E_DD32_4053_BFFE_F4A79D3AA525__INCLUDED_)

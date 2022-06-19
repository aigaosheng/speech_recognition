#ifndef CONST_H
#define CONST_H
#include	<math.h>

// Constants for Model
#define MAX_WORD_LENGTH    7

#define BLOCKNUM    5//8//           // Multithreshold Pruning

#define NSLmax 60000//120000//16000//              // The depth of stack
#define log_base 1.0001       	 // For viterbi log scaling
#define neg_huge -1000000000L
#define pos_huge  1000000000L

/////////////////////////////////////////////
//1425个声母triphone模型，1266个声母输出分布
#define Max_State				6
#define Max_Out					4
#define Max_Trans				9
#define	MAX_NODE_TRI			14000//6000//4000//6000
#define	TOTAL_TRIPHONES			13307//8327//13470//11973//10801//5342//4901//4734//4542//303//5442//2648
#define	TRI_SILENCE_CODE		TOTAL_TRIPHONES-1
#define	TRI_NO_HMM				13307//8327//13470//11973//10801//5342//4901//4734//4542//303//5442//2648
#define	No_Output				2733//2872//2461//2704//2591//3063//2684//2581//2646//974//3213//1647

#define No_Dim	42//39//
#define	No_Mix	16//2//8//4
#define FEATURE_DIM	14//13//13

#define MAX_CODE_INITIAL_PART	23//19
#define	MAX_CODE_FINAL_PART		60//72
#define TOTAL_INITIAL	24
#define TOTAL_FINAL		37

#define MAX_SPEECH_LENGTH  2000//1200//800
#define	TOTAL_SYLLABLES		409//408//668
#define	TOTAL_PHONES		62//74
#define	SILENCE_CODE		61//73

#define	MAX_NUM_PHONE		200
#define	MAX_NUM_WORD		60
#define	MAX_SENTENCE_LENGTH	60


#define FFTLen		512
#define FiltNum		25
#define FS			16.0
#define PI			3.1415926536
// Constants for PreProcessing
#define 	pre			0.97
#define 	hFrameLen	192
#define 	FrameLen	384

// Constants for Feature
#define 	PCEP		12
#define 	PEDE		3


#define		NO_WORDS	39969
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
//for path hash
#define PENALTY 0//-10000//
#define MODULE_VALUE1	809
#define MODULE_VALUE2	3001
#define MAX_BUCKET_LIST	1000

//以下宏定义用于指数函数查表
//#define	CHECK_EXP_TABLE
#define	EXP_STEP	100//0.001
#define	EXP_MAX_VAL	400
#define	EXP_TABLE_SIZE	(EXP_MAX_VAL * EXP_STEP)
//
//const double logbase = 10*log(1.000005);
const double logbase = 20000.049999958333437499670140061;
const double pai2 = 8*atan(1.0);
//高斯快速计算的码本大小
#define	CODE_BOOK_SIZE	256//512
//
#define INTRA_WORD_THRESHOLD	1000//1200//2000//800//
#define INTER_WORD_THRESHOLD	800//3000//1000//500//
#define	NEG_LM	65535//-19980049

//路径的门限
/*
long M_EPS[BLOCKNUM]={200000,300000,400000, 600000, 800000,
					   1000000, 1200000, 1500000
					  };
*/
const long M_EPS_INTRA[BLOCKNUM]={400000, 600000, 800000, 900000, 1000000
					  };
const long M_EPS_INTER[BLOCKNUM]={600000, 650000, 700000, 800000, 1000000
					  };
const long maxOffset = 1000000;
/*
long M_EPS[BLOCKNUM]={600000L,700000L,800000L,
					   1000000L,1100000L,1200000L,1300000L,1400000L
					  };
*/


#endif
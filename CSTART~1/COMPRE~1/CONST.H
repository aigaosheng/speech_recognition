#ifndef CONST_H
#define CONST_H

// Constants for Model
#define MAX_WORD_LENGTH    7

#define BLOCKNUM    8            // Multithreshold Pruning

#define NSLmax 60000//120000//16000//              // The depth of stack
#define log_base 1.0001       	 // For viterbi log scaling
#define neg_huge -1000000000L
#define pos_huge  1000000000L

/////////////////////////////////////////////

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

#define MAX_SPEECH_LENGTH  1200//800
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

#define	PITCH_0		0 //ÇáÉù
#define	PITCH_1		1 //1Éù
#define	PITCH_2		2 //2Éù
#define	PITCH_3		3  //3Éù
#define	PITCH_4		4 //4Éù
#define	PITCH_SIL0	5 //ÇáÉù
#define	PITCH_SIL1	6 //1Éù
#define	PITCH_SIL2	7 //2Éù
#define	PITCH_SIL3	8 //3Éù
#define	PITCH_SIL4	9 //4Éù
#define	PITCH_SIL	10 //Ã»ÓÐÉùµ÷,¼´²»¿¼ÂÇÉùµ÷
#define	PITCH_NULL	5 //²»¿¼ÂÇÉùµ÷
//for predict-LM cache
#define MAX_CACHE_LIST	10
#define MAX_TREE_NODE	120000
//for path hash
#define PENALTY 0//-10000//
#define MODULE_VALUE1	809
#define MODULE_VALUE2	3001
#define MAX_BUCKET_LIST	1000

#endif
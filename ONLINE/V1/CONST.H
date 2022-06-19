// Const.h: interface for the CConst class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONST_H__EF9C4F9E_DD32_4053_BFFE_F4A79D3AA525__INCLUDED_)
#define AFX_CONST_H__EF9C4F9E_DD32_4053_BFFE_F4A79D3AA525__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <limits.h>
/////////////////////////////////////////////////////////////////////////////////////////
// 从原来的const.h中复制过来的

#define	WM_DRAGONVOICE_FIRM		(WM_USER+11)
#define	WM_DRAGONVOICE_GETVOLUME	(WM_USER+12)
#define	WM_DRAGONVOICE_ADAPT_NEXT	(WM_USER+13)

//与特征参数提取有关的常量
#define 	PCEP		12 //MFCC的维数
#define 	PEDE		3  //
#define		FFTLEN		512 //FFT的点数
#define		FILTNUM		25 //低通滤波器的数目
#define		FS			16.0 //采样率=*1000
#define		PI			3.1415926536
#define 	PRE			0.97 //预加重系数
#define 	HFRAMELEN	192 //半帧长度，采样点
#define 	FRAMELEN	384 //一帧的长度，采样点

#define 	FHZ				16000 //采样率
#define 	BITS			16 //采样点编码位数
#define 	BYTESIZE		32000 // fhz*bits/8

//与特征向量有关的常量
#define		NO_DIM			42//39//	//特征向量的维数
#define		NO_DIM_ALIGNED	44 / 4
#define		NO_MIX			16//2//8//4
#define		FEATURE_DIM		14//13//13	//于是数据的维数
//与声学模型模型拓扑结构有关的常量
#define MAX_STATE				6
#define MAX_OUT					4
#define MAX_TRANS				9
//与三音子声学模型有关的系数，声学模型不同，以下参数不同
//#define	TOTAL_TRIPHONES			13307 //13453//7272//5942//模型数目
//#define	TRI_SILENCE_CODE		TOTAL_TRIPHONES-1 //静音模型的编码
//#define	TRI_NO_HMM				13307 //13453//7272//5942//模型数目
//#define	NO_OUTPUT				2733 //2788//2045//1632//输出分布的数目
//#define	MAX_NODE_TRI			14000//6000//4000//6000

//与语言模型有关的常量，编码与词典和词树有关
#define		MAX_USER_DIC	5000

#define		HEAD_WORD	13625
#define		TAIL_WORD	13625
#define		NO_WORDS	39969//词典中词的个数
#define		MAX_WORD_LENGTH    7	//词典中每个词的最大长度
//语言模型和声学模型的权重
#define		ACOUSTIC_WEIGHT	1
#define		LANGUAGE_WEIGHT_WORD  10//15	
#define		LANGUAGE_WEIGHT	10//10

#define		MAX_WORD_CANDIDATES	 20 //候选词对的数目

//设置最小和最大值
//#define LOG_BASE 1.0001       	 // For viterbi log scaling
#define NEG_HUGE -2000000000L
#define POS_HUGE  LONG_MAX//1000000000L
#define	GAUSSIAN_BOTTOM	-800000

//设置与base phone有关的常量
#define MAX_CODE_INITIAL_PART	23//19
#define	MAX_CODE_FINAL_PART		60//72
#define	TOTAL_SYLLABLES		409//408//668
#define TOTAL_INITIAL		24	//ke wen
#define TOTAL_FINAL			37	//ke wen
#define	TOTAL_PHONES		62//74
#define	SILENCE_CODE		61//73
//有调模型的常量
#define	TOTAL_PITCH_PHONES		306  //24*5+37*5+1
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

//与buffer有关的系数
#define 	FRAMESPERBUF			20 //每个录音块的帧数
#define 	WAVEBUFSIZE				(FRAMESPERBUF * HFRAMELEN) //每个录音块的点数
#define  	WAVEBUFNUM				25 //为录音设备分配的buffer大小

#define		MAX_BLOCK_START_RECOGNIZER	10//30//20  //开始特帧提取的录音块数，帧数=*每个录音块的数目
#define		MAXFIFOITEMSIZE			(80*MAX_BLOCK_START_RECOGNIZER * FRAMESPERBUF*6) //FIFO的大小（帧数）
#define		MAXPROCESSFRAMENUM		(MAX_BLOCK_START_RECOGNIZER * FRAMESPERBUF) //开始识别处理的帧数
#define		MAX_WORD_LATTICE_STACK	5000//2000//600 //有词对出现的时间长度
#define		MAX_CONTINUOUS_TIME		5000//3000 //最长语音持续时间
#define		MAX_SPEECH_LENGTH		1000 //最长的句子帧数

//FIFO的返回值
#define		FIFO_OPERATION_NORMAL	1
#define		FIFO_OPERATION_ERROR	0
#define		FIFO_NULL				2
#define		FIFO_FLOW				3	
#define		FIFO_OPERATION_SPEECH_SEGMENT_END 4

//路径管理
#define		MODULE_VALUE1	4999//809//8089//Hash表1的模值
#define		MODULE_VALUE2	14983//3001//29989//Hash表2的模值
//
#define		NSLMAX 30000//60000//120000//16000//  路径的最大堆栈
//路径裁剪
#define		INTRA_WORD_THRESHOLD	1000//1200//2000//800//
#define		INTER_WORD_THRESHOLD	800//3000//1000//500//
#define		BLOCKNUM    5            // Multithreshold Pruning
//与录音模块有关的系数
#define		MAX_BEGINE_AMPLITUDE	4 //录音端点检测，有语音能量阀值
#define		MAX_END_AMPLITUDE		4//3 //录音端点检测，语音结束能量阀值
#define		MINSPEECHLENGTH			10
//
#define		bbase 10

//以下常量用于局部回溯,高升编辑
#define		MAX_CONFIRM_BLOCK	10//50//30//100//3//2//
#define		MAX_TRACE_TIME_SLOT	10//2//2//1//20//10//50//
#define		MIN_TRACE_START_TIME 100//50//120//200//40
//
#define	CODE_BOOK_SIZE	256//512 //高斯快速计算码本的大小
//语言模型的cache
#define	MODE_VALUE_LM_CACHE	3001//MAX_TREE_NODE//2005//203 
#define MAX_CACHE_LIST	10//5//20//5//
#define MAX_TREE_NODE	120000

//设置训练句子中phone的数目和句子的词的个数
#define	MAX_NUM_PHONE		200
#define	MAX_NUM_WORD		60
#define	MAX_SENTENCE_LENGTH	1000//60
#define MAX_SENTENCE_PARTIAL_LENGTH	200
//
#define	MAX_SENTENCE_CHARACTERS	500
//
#define	MODE_VALUE_SECOND	20
#define	MODE_VALUE_LM_CACHE_SECOND	1300
//
#endif // !defined(AFX_CONST_H__EF9C4F9E_DD32_4053_BFFE_F4A79D3AA525__INCLUDED_)

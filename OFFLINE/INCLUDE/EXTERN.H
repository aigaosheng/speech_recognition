#ifndef	_EXTERN_H
#define	_EXTERN_H

#include	"include\struct.h"
#include	"include\const.h"

//外部变量声明
extern int speechDataLength;
extern float pVector[MAX_SPEECH_LENGTH][No_Dim];

//全局变量声明
extern int		mapping[TOTAL_TRIPHONES];//class-triphone与base phone的映射
//存放声学模型参数
extern COutput	Output[No_Output]; //输出分布
extern CHmm		Hmm[TRI_NO_HMM]; //模型参数

//存放和索引高斯快速计算的数据库
extern short	*shortListNum; //高斯快速计算的short list表
extern int		*shortListOffset; //高斯快速计算的索引
extern char		*shortList; //高斯快速计算的short list表

//存放高斯快速计算的码本均值和方差
extern float	*clusterMean, //均值
				*clusterVar; //方差
//指数查找表
extern double	*expTable;
//语言的变量
extern unsigned char	*pHanzi; //汉字字库
extern unsigned char	*pUnigram; //一元概率
extern unsigned char	*pBigram,  //二元概率
						*pBigramIdx;
extern unsigned char	*pTrigram,	//三元概率
						*pTrigramIdx;
extern long				*pLmCodebook; //语言模型量化得到的码本
//词树指针
extern CLexicalTree		*pLexicalTree;
//由上下文查找class-triphone所需数据库
extern short	*pTriphoneMapInitial, 
				*pTriphoneMapFinal;

extern long		ProbBuffer[No_Output]; //高斯概率缓存

//路径的总堆栈
extern CPhoneNode	tokenPath[NSLmax];
//hash表管理
extern int		pBucketHead[2][MODULE_VALUE1+MODULE_VALUE2]; //头指针
//动态内存的堆栈管理
extern int		freeBlockIdx[NSLmax], //空闲数据块
				freeBlockTop; //栈顶指针

extern long maxProbThreshold;
//词的lattice管理
extern CWordLattice	wordLattice[MAX_SPEECH_LENGTH][MAX_WORD_CANDIDATES];
extern int			wordNumber[MAX_SPEECH_LENGTH];
//回溯
extern int senLen, wordSeq[100], timeSeq[100], posSeq[100];

//语言模型预测概率的cache
extern CDisLmCache	lmCache[MAX_TREE_NODE][MAX_CACHE_LIST];
extern short		pLmCacheHead[MAX_TREE_NODE];
extern char			lmCacheFull[MAX_TREE_NODE];

#endif
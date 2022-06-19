#include	"include\const.h"
#include	"include\struct.h"

//全局变量声明
//
int		speechDataLength;
float	pVector[MAX_SPEECH_LENGTH][No_Dim];
//
int		mapping[TOTAL_TRIPHONES];//class-triphone与base phone的映射
//存放声学模型参数
COutput	Output[No_Output]; //输出分布
CHmm	Hmm[TRI_NO_HMM]; //模型参数

//存放和索引高斯快速计算的数据库
short	*shortListNum; //高斯快速计算的short list表
int		*shortListOffset; //高斯快速计算的索引
char	*shortList; //高斯快速计算的short list表

//存放高斯快速计算的码本均值和方差
float	*clusterMean, //均值
		*clusterVar; //方差
//指数查找表
double *expTable;
//语言的变量
unsigned char	*pHanzi; //汉字字库
unsigned char	*pUnigram; //一元概率
unsigned char	*pBigram,  //二元概率
				*pBigramIdx;
unsigned char	*pTrigram,	//三元概率
				*pTrigramIdx;
long			*pLmCodebook; //语言模型量化得到的码本
//词树指针
CLexicalTree *pLexicalTree;
//由上下文查找class-triphone所需数据库
short	*pTriphoneMapInitial, 
		*pTriphoneMapFinal;

long	ProbBuffer[No_Output]; //高斯概率缓存

//路径的总堆栈
CPhoneNode	tokenPath[NSLmax];
//hash表管理
int		pBucketHead[2][MODULE_VALUE1+MODULE_VALUE2]; //头指针
//动态内存的堆栈管理
int		freeBlockIdx[NSLmax], //空闲数据块
		freeBlockTop; //栈顶指针
//
int flag1, flag2;
long maxProbThreshold;

//词的lattice管理
CWordLattice	wordLattice[MAX_SPEECH_LENGTH][MAX_WORD_CANDIDATES];
int				wordNumber[MAX_SPEECH_LENGTH];
//回溯
int senLen, wordSeq[100], timeSeq[100], posSeq[100];

//语言模型预测概率的cache
CDisLmCache	lmCache[MAX_TREE_NODE][MAX_CACHE_LIST];
short		pLmCacheHead[MAX_TREE_NODE];
char		lmCacheFull[MAX_TREE_NODE];

//THMMParam	 *Hmm;

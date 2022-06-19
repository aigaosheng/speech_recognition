/**************************************************************************************************************
Description: The search engine is one pass decoder based on Lexical Tree and LM look-ahead technique.
Written By: Gao sheng      
Date: Jan 20,2000
Date modified: April 3,2000
			   June 1,2000
Note: modified because pitch information is used in speech feature. Models are pitch-considered triphones.
Modified Data: June 21,2000
Modified Note: use hash table and dynamically organize paths that means releasing memory of deactive path.
//
1. 对Trigram索引重新表示 6
2. 产生语言模型量化的码本 0
3. 对语言模型量化 1
4. 对Trigram的二级索引压缩 2
5. 检查索引压缩的正确性 3
6. 对Bigram的词号进行压缩 4
7. 对Trigram的词号进行压缩 5
**************************************************************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>
#include	<assert.h>
#include	<float.h>
#include	<string.h>
#include	<process.h>
#include	<conio.h>
#include	<limits.h>

#define		MAX_WORD_LENGTH	7
#define		NO_WORDS	39969

#define		CODEWORD_SIZE	unsigned char

#define LOAD_TRIGRAM_SECOND_INDEX


unsigned char	*pHanzi;
unsigned char	*pUnigram;
unsigned char	*pBigram,
				*pBigramIdx;
unsigned char	*pTrigram,
				*pTrigramIdx[2];
unsigned int	*pTrigramIdxOffset;

#define		NEG_LM	-19980049
#define		NEG_LM_NEW	65535
#define		NEG_LM_BYTE	255
#define		neg_huge -1000000000L
#define		pos_huge 1000000000L
#define		VQ_REF_VALUE	171770
/////////////////
typedef struct{
		int left;
		int right;
		int weight;
		int parent;
		char mark;
} HuffmanNode;
//////////////
int LoadData(int);
int LoadUnigram(int);
int LoadBigram(int);
int LoadTrigram(int);
void	ReleaseMemory(void);
//long CheckTrigram(unsigned short w1, unsigned short w2, unsigned short w3);
//long CheckTriBackoff(unsigned short w1, unsigned short w2);
//long CheckBigram(unsigned short w1, unsigned short w2);
extern void	BinaryClusterLm(double *pSample, long sampleNum, long ref);
void	ClusterLm(void);
void	VqLm(void);
void	AnalizeLmIdx(void);
void	CheckCompressIndex(void);
void	BigramHuffmanCode(void);
void	TrigramHuffmanCode(void);
void	TrigramHuffmanCode2(void);

void	AnalizeLmProb(void);
void	GenerateHuffmanTree(int, int*, char*);
void	GenerateBigramHuffmanTree(void);
void	GenerateTrigramHuffmanTree(void);
void	GenerateTrigramHuffmanTree2(void);
void	HuffmanCoder(int curidx, HuffmanNode* node, unsigned char *codebit, int &bitnum);
unsigned short	HuffmanDecoder(int rootnode, HuffmanNode* node, unsigned char *codebit);
unsigned short	HuffmanDecoder2(int rootnode, HuffmanNode* node, unsigned char *codebit, long& offset);
void	ReorderBigram(void);
void	ReorderTrigram(void);
void	ReorderTrigram2(void);


unsigned short FindCodeword(double*, int, double);

int FindTrigramIdxOffset(int offset, unsigned int* m_pTrigramIdxOffset, int len);

extern void	ProcessTrigram(void);

int	 uniGramNum, biGramNum, biGramIdxNum, triGramNum, trigramIdxNum1, trigramIdxNum2, trigramOffsetNum;

void main(int argv, char* argc[])
{
	if(argv != 2)
	{
		printf("Need one argument:\n");
		printf("Argument is '0': Cluter and Generate codebook\n");
		printf("Argument is '1': quantize LM\n");
		printf("Argument is '2': Compress Trigram Index2\n");
		printf("Argument is '3': Check Compressed Trigram Index2\n");
		printf("Argument is '4': 压缩Bigram词号\n");
		printf("Argument is '5': 压缩Trigram词号\n");
		printf("Argument is '6': 转换Trigram的索引表示\n");
		//
		printf("建议使用步骤：\n");
		printf("	1. 对Trigram索引重新表示 6\n");
		printf("	2. 产生语言模型量化的码本 0\n");
		printf("	3. 对语言模型量化 1\n");
		printf("	4. 对Trigram的二级索引压缩 2\n");
		printf("	5. 检查索引压缩的正确性 3\n");
		printf("	6. 对Bigram的词号进行压缩 4\n");
		printf("	7. 对Trigram的词号进行压缩 5\n");

		return;
	}
	int argument = atoi(argc[1]);
	switch(argument)
	{
		case 0:
			//读取样本
			LoadData(0);
			ClusterLm();//聚类产生码本
			break;
		case 1:
			//读取样本
			LoadData(1);
			VqLm();//对语言模型进行量化
			break;
		case 2:
			//读取样本
			LoadData(2);
			AnalizeLmIdx(); //Trigram索引压缩
			break;
		case 3:
			//读取样本
			LoadData(3);
			CheckCompressIndex(); //检查Trigram索引压缩的正确与否
			break;
		case 4:
			//读取样本
			LoadData(5); //读取LM矢量量化和偏移量压缩后的数据
			ReorderBigram(); //根据新词号重新排序，产生词号增量的Bigram文件
			GenerateBigramHuffmanTree(); //对词号增量进行Huffman编码
			BigramHuffmanCode(); //压缩
			break;
		case 5:
			//读取样本
			LoadData(5); //读取LM矢量量化和偏移量压缩后的数据
			ReorderTrigram(); //根据新词号重新排序，产生词号增量的Bigram文件
			GenerateTrigramHuffmanTree(); //对词号增量进行Huffman编码
			TrigramHuffmanCode(); //压缩
			//ReorderTrigram2(); //根据新词号重新排序，产生词号增量的Bigram文件
			//GenerateTrigramHuffmanTree2(); //对词号增量进行Huffman编码
			//TrigramHuffmanCode2(); //压缩
			break;
		case 6:
			ProcessTrigram();
			break;
		case 7:
			//读取样本
			LoadData(5);
			AnalizeLmProb();
			break;
		default:
			printf("Argument must be '0': Cluter and Generate codebook)\n");
			printf("				 '1': quantize LM\n");
			printf("				 '2': Compress Trigram Index2\n");
			printf("				 '3': Check Compressed Trigram Index2\n");
			printf("				 '4': 压缩Bigram词号\n");
			printf("				 '5': 压缩Trigram词号\n");
			break;
	}
	ReleaseMemory();
}

int LoadData(int type)
{
	if(LoadUnigram(type))
	{
		printf("unigram not found\n");
		getch();
		exit(-1);
	}
	if(LoadBigram(type))
	{
		printf("bigram not foud\n");
		getch();
		exit(-1);
	}
	if(LoadTrigram(type))
	{
		printf("trigram fot found\n");
		getch();
		exit(-1);
	}
	return 0;
}


int LoadUnigram(int type)
{
	FILE *funigram;
	int i, j;
	char str[MAX_WORD_LENGTH*2+1];

	//funigram = fopen("data\\gs_80_unigram.lm", "rb");
	if(type != 5)
		//funigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\dataNoRightpitch\\gs_80_unigram.lm", "rb");
		funigram = fopen("gs_80_unigram.lm", "rb");
	else
//		funigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\vq.lm", "rb");	
		funigram = fopen("vq.lm", "rb");	

	if(funigram == NULL)
	{
		printf("unigram file open error\n");
		return 1;
	}
	fseek(funigram, 0L, SEEK_END);
	i = ftell(funigram);
	pUnigram = new unsigned char[i];
	if(pUnigram == NULL)
	{
		printf("memory is not sufficient2\n");
		fclose(funigram);
		return 2;
	}
	//
	if(type != 5)
		uniGramNum = i / 8;
	else
		uniGramNum = i / 3;

	fseek(funigram, 0L, SEEK_SET);
	fread(pUnigram, sizeof(unsigned char), i, funigram);
	fclose(funigram);
	//Load dictionary
	funigram = fopen("hanzi.han", "rt");
	//funigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\dataNoRightpitch\\hanzi.han", "rt");
	if(funigram == NULL)
	{
		printf("dictionary not found\n");
		delete pUnigram;
		exit(-1);
	}
	pHanzi = new unsigned char[NO_WORDS*MAX_WORD_LENGTH*2];
	if(pHanzi == NULL)
	{
		printf("memory is not sufficient\n");
		fclose(funigram);
		delete pUnigram;
		return 2;
	}
	long offset = 0;
	for(i = 0; i < NO_WORDS; i++)
	{
		fscanf(funigram, "%s", str);
		for(j = 0; j < MAX_WORD_LENGTH * 2; j++)
			pHanzi[offset+j] = str[j];
		offset += MAX_WORD_LENGTH * 2;
	}
	fclose(funigram);

	return 0;
}

int LoadBigram(int type)
{
	FILE *fbigram;
	long i;

	//Load bigram index
	if(type != 5)
		//fbigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\dataNoRightpitch\\gs_bkoff_80.bdx", "rb");
		fbigram = fopen("gs_bkoff_80.bdx", "rb");
	else
//		fbigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\gs_bkoff_80.bdx", "rb");
		fbigram = fopen("gs_bkoff_80.bdx", "rb");

	if(fbigram == NULL)
	{
		printf("bigram index file open error\n");
		return 1;
	}
	fseek(fbigram, 0L, SEEK_END);
	i = ftell(fbigram);
	biGramIdxNum = i / 6;
	pBigramIdx = new unsigned char[i];
	if(pBigramIdx == NULL)
	{
		printf("memory is not sufficient2\n");
		fclose(fbigram);
		return 2;
	}
	fseek(fbigram, 0L, SEEK_SET);
	fread(pBigramIdx, sizeof(unsigned char), i, fbigram);
	fclose(fbigram);
	//Load bigram
	if(type != 5)
		//fbigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\dataNoRightpitch\\gs_bkoff_80.bgm", "rb");
		fbigram = fopen("gs_bkoff_80.bgm", "rb");
	else
//		fbigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\vq.bgm", "rb");
		fbigram = fopen("vq.bgm", "rb");

	if(fbigram == NULL)
	{
		printf("bigram index file open error\n");
		return 1;
	}
	fseek(fbigram, 0L, SEEK_END);
	i = ftell(fbigram);

	fseek(fbigram, 0L, SEEK_SET);
	pBigram = new unsigned char[i];
	if(pBigram == NULL)
	{
		printf("memory is not sufficient3\n");
		fclose(fbigram);
		return 2;
	}
	if(type != 5)
		biGramNum = i / 10;
	else
		biGramNum = i / 4;

	fread(pBigram, sizeof(unsigned char), i, fbigram);
	fclose(fbigram);
	return 0;
}

#ifndef LOAD_TRIGRAM_SECOND_INDEX
int LoadTrigram()
{
	FILE *ftrigram;
	long i;

	//Load trigram index
//	ftrigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\gs_bkoff_80.tdx", "rb");
	ftrigram = fopen("gs_bkoff_80.tdx", "rb");
	if(ftrigram == NULL)
	{
		printf("trigram index file open error\n");
		return 1;
	}
	fseek(ftrigram, 0L, SEEK_END);
	i = ftell(ftrigram);
	pTrigramIdx[0] = new unsigned char[i];
	if(pTrigramIdx[0] == NULL)
	{
		printf("memory is not sufficient2\n");
		fclose(ftrigram);
		return 2;
	}

	fseek(ftrigram, 0L, SEEK_SET);
	fread(pTrigramIdx[0], sizeof(unsigned char), i, ftrigram);
	fclose(ftrigram);
	//Load trigram
	//ftrigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\gs_bkoff_80.tgm", "rb");
	ftrigram = fopen("gs_bkoff_80.tgm", "rb");
	if(ftrigram == NULL)
	{
		printf("trigram index file open error\n");
		return 1;
	}
	fseek(ftrigram, 0L, SEEK_END);
	i = ftell(ftrigram);
	fseek(ftrigram, 0L, SEEK_SET);
	pTrigram = new unsigned char[i];
	if(pTrigram == NULL)
	{
		printf("memory is not sufficient3\n");
		fclose(ftrigram);
		return 2;
	}
	triGramNum = i / 8;
	fread(pTrigram, sizeof(unsigned char), i, ftrigram);
	fclose(ftrigram);

	return 0;
}

#else
//Trigram二级索引
int LoadTrigram(int type)
{
	FILE *ftrigram;
	long i;

	//Load trigram index
//	ftrigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\dataNoRightpitch\\trigram1.idx", "rb");
	ftrigram = fopen("gs_bkoff_80.tdx", "rb");
	if(ftrigram == NULL)
	{
		printf("trigram index file open error\n");
		return 1;
	}
	fseek(ftrigram, 0L, SEEK_END);
	i = ftell(ftrigram);
	trigramIdxNum1 = i / 6;
	pTrigramIdx[0] = new unsigned char[i];
	if(pTrigramIdx[0] == NULL)
	{
		printf("memory is not sufficient2\n");
		fclose(ftrigram);
		return 2;
	}

	fseek(ftrigram, 0L, SEEK_SET);
	fread(pTrigramIdx[0], sizeof(unsigned char), i, ftrigram);
	fclose(ftrigram);
	//Load trigram index
	if(type!=5)
//		ftrigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\dataNoRightpitch\\trigram2.idx", "rb");
		ftrigram = fopen("trigram2.idx", "rb");
	else
//		ftrigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\tricmp2.idx", "rb");
		ftrigram = fopen("tricmp2.idx", "rb");

	if(ftrigram == NULL)
	{
		printf("trigram index file open error\n");
		return 1;
	}
	fseek(ftrigram, 0L, SEEK_END);
	i = ftell(ftrigram);
	if(type!=5)
		trigramIdxNum2 = i / 8;
	else
		trigramIdxNum2 = i / 3;

	pTrigramIdx[1] = new unsigned char[i];
	if(pTrigramIdx[1] == NULL)
	{
		printf("memory is not sufficient2\n");
		fclose(ftrigram);
		return 2;
	}

	fseek(ftrigram, 0L, SEEK_SET);
	fread(pTrigramIdx[1], sizeof(unsigned char), i, ftrigram);
	fclose(ftrigram);
///装载Trigram Index offset database
	if(type==5)
	{
//		ftrigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\trioffset2.idx", "rb");
		ftrigram = fopen("trioffset2.idx", "rb");
		if(ftrigram == NULL)
		{
			printf("trigram index file open error\n");
			return 1;
		}
		fseek(ftrigram, 0L, SEEK_END);
		trigramOffsetNum = ftell(ftrigram) / sizeof(unsigned int);

		pTrigramIdxOffset = new unsigned int[trigramOffsetNum];
		if(pTrigramIdxOffset == NULL)
		{
			printf("memory is not sufficient2\n");
			fclose(ftrigram);
			return 2;
		}
		fseek(ftrigram, 0L, SEEK_SET);
		fread(pTrigramIdxOffset, sizeof(unsigned int), trigramOffsetNum, ftrigram);
		fclose(ftrigram);
	}

	//Load trigram
	if(type != 5)
//		ftrigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\dataNoRightpitch\\trigram.tgm", "rb");
		ftrigram = fopen("trigram.tgm", "rb");
	else
//		ftrigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\vq.tgm", "rb");
		ftrigram = fopen("vq.tgm", "rb");

	if(ftrigram == NULL)
	{
		printf("trigram index file open error\n");
		return 1;
	}
	fseek(ftrigram, 0L, SEEK_END);
	i = ftell(ftrigram);
	fseek(ftrigram, 0L, SEEK_SET);
	pTrigram = new unsigned char[i];
	if(pTrigram == NULL)
	{
		printf("memory is not sufficient3\n");
		fclose(ftrigram);
		return 2;
	}
	if(type != 5)
		triGramNum = i / 6;
	else
		triGramNum = i / 3;

	fread(pTrigram, sizeof(unsigned char), i, ftrigram);
	fclose(ftrigram);

	return 0;
}

void	AnalizeLmIdx()
{
	int i, j;

	
	/////压缩TrigramIdx2中的offset
	///去掉索引文件中的num，而由偏移量计算
	unsigned char *l = new unsigned char[trigramIdxNum2+1];
	unsigned int  *h = new unsigned int[trigramIdxNum2+1];
	for(i = 0; i < trigramIdxNum2; i++)
	{
		l[i] = *(long*)(pTrigramIdx[1] + i * 8 + 4) % 256;
		h[i] = *(long*)(pTrigramIdx[1] + i * 8 + 4) / 256;
	}
	int tmpint = *(long*)(pTrigramIdx[1] + (trigramIdxNum2-1) * 8 + 4) + *(unsigned short*)(pTrigramIdx[1] + (trigramIdxNum2-1) * 8 + 2);
	l[trigramIdxNum2] = tmpint % 256;
	h[trigramIdxNum2] = tmpint / 256;

	unsigned int *r = new unsigned int[trigramIdxNum2+1];

	r[0] = 0;
	j = 0;
	i = 0;
	while(i <= trigramIdxNum2)
	{
		if(h[i] == j)
		{
			i++;
			continue;
		}
		else
		{
			j++;
			r[j] = i;
		}
	}
	j++;
	r[j] = i;
	j++;

	FILE *fp = fopen("trioffset2.idx", "wb");
	if(fp == NULL)
	{
		printf("create file error\n");
	}
	fwrite(r, sizeof(unsigned int), j, fp);
	fclose(fp);

	fp = fopen("tricmp2.idx", "wb");
	if(fp == NULL)
	{
		printf("create file error\n");
	}
	for(i = 0; i < trigramIdxNum2; i++)
	{
		fwrite((unsigned short*)(pTrigramIdx[1] + i * 8), sizeof(unsigned short), 1, fp);
		fwrite(l+i, sizeof(unsigned char), 1, fp);
	}

	unsigned short tmp = 65535;
	fwrite(&tmp, sizeof(unsigned short), 1, fp);
	fwrite(l+trigramIdxNum2, sizeof(unsigned char), 1, fp);
	fclose(fp);

	//检查压缩是否有错误
	for(i = 0; i < trigramIdxNum2; i++)
	{
		int idx = FindTrigramIdxOffset(i, r, j);
		int offsettmp = ((idx - 1) << 8) + l[i];

		if(offsettmp != (*(long*)(pTrigramIdx[1] + i * 8 + 4)))
		{
			printf("error when compress\n");
			getch();
			break;
		}
	}
	//
	delete l;
	delete h;
	delete r;
	//////////////////////////
}
void	CheckCompressIndex()
{
	int i, j;

	
	/////压缩TrigramIdx2中的offset
	///去掉索引文件中的num，而由偏移量计算
	unsigned int *r = new unsigned int[trigramIdxNum2+1];

	FILE *fp = fopen("trioffset2.idx", "rb");
	if(fp == NULL)
	{
		printf("create file error\n");
	}
	fseek(fp, 0, SEEK_END);
	j = ftell(fp) / 4;
//	assert(len == trigramIdxNum2 + 1);
	fseek(fp, 0, SEEK_SET);
	fread(r, sizeof(unsigned int), j, fp);
	fclose(fp);

	fp = fopen("tricmp2.idx", "rb");
	if(fp == NULL)
	{
		printf("create file error\n");
	}
	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);
	assert(len / 3 == trigramIdxNum2 + 1);
	fseek(fp, 0, SEEK_SET);
	unsigned char *ptmp = new unsigned char[len];
	fread(ptmp, sizeof(unsigned char), len, fp);
	fclose(fp);
	//
	for(int k = 0; k < uniGramNum; k++)
	{
		int offset = *(long*)(pTrigramIdx[0] + k * 6+2);
		for(i = 0; i < (*(unsigned short*)(pTrigramIdx[0] + k * 6)); i++)
		{
			int	offsetidx1 = FindTrigramIdxOffset(offset + i, r, j);
			int	offsetidx2 = FindTrigramIdxOffset(offset + i + 1, r, j);
			
			offsetidx1 = ((offsetidx1 - 1) << 8) + (*(unsigned char*)(ptmp + (offset+i) * 3 + 2));
			offsetidx2 = ((offsetidx2 - 1) << 8) + (*(unsigned char*)(ptmp + (offset+i+1) * 3 + 2));
			int num2 = offsetidx2 - offsetidx1;

			if((*(unsigned short*)(ptmp + (offset+i) * 3)) != (*(unsigned short*)(pTrigramIdx[1] + (i+offset) * 8)))
			{
				printf("error when compress\n");
				getch();
				break;
			}
			if(offsetidx1 != (*(long*)(pTrigramIdx[1] + (i+offset) * 8 + 4)))
			{
				printf("error when compress\n");
				getch();
				break;
			}
			if(num2 != (*(unsigned short*)(pTrigramIdx[1] + (i+offset) * 8 + 2)))
			{
				printf("error when compress\n");
				getch();
				break;
			}
		}
	}
	//
	delete r;
	//////////////////////////
}

int FindTrigramIdxOffset(int offset, unsigned int* m_pTrigramIdxOffset, int len)
{
	int low = 0;
	int high = len-1;
	int mid;
	unsigned char bFound = 0;

	while( low <= high ) 
	{
		mid = (low + high) / 2;

		if (m_pTrigramIdxOffset[mid] == offset)
		{
			if(m_pTrigramIdxOffset[mid+1] > offset)
			{
				mid = mid + 1;
				bFound = 1;
				break;
			}
			low = mid + 1;
		}
		else if (m_pTrigramIdxOffset[mid] > offset) 
		{
			high = mid-1; //continue in low interval
			if(m_pTrigramIdxOffset[high] <= offset)
			{
				bFound = 1;
				break;
			}
		}
		else 
		{
			low = mid + 1; //continue in high interval
			if(m_pTrigramIdxOffset[low] > offset)
			{
				bFound = 1;
				mid = mid + 1;
				break;
			}
		}
	}
	if(!bFound)
		return -1;
	else
		return mid;
}

#endif

//产生Huffman编码树
void	GenerateHuffmanTree(int wordNumber, int *wordcount, char *fileHuffman)
{
	int i, totalcount = 0;
	int index;	
	FILE *fpp;

	HuffmanNode *node = new HuffmanNode[2*wordNumber-1];
	int *usedSpace = new int[2*wordNumber-1], usedTop = 0;
	int *freeSpace = new int[2*wordNumber-1], freeTop = 0;
	//
	for(i = 0; i < 2*wordNumber-1; i++)
	{
		freeSpace[2*wordNumber-2-i] = i;
		freeTop++;
		//
		node[i].left = -1;
		node[i].right = -1;
		node[i].weight = -1;
		node[i].mark = 0;
		node[i].parent = -1;
	}
	
	//fpp=fopen("wordcount.txt","wt");

	int codedWordNum = 0;
	unsigned short wordcodeMap[NO_WORDS];
	for(i = 0; i < wordNumber; i++)
	{
		if(wordcount[i] > 0) 
		{
			index = freeSpace[freeTop-1];
			freeTop--;
			node[index].left = i;
			node[index].right = -1;
			node[index].weight = wordcount[i];
			node[index].mark = 0;
			node[index].parent = -1;
			wordcodeMap[i] = index;
			//
			usedSpace[usedTop] = i;
			usedTop++;
			codedWordNum++;
			//
	//		fprintf(fpp, "%d\n", wordcount[i]);
		}
		else
		{
			wordcodeMap[i] = 0xffff;
		}
		totalcount += wordcount[i];
	}
	//fclose(fpp);
	
	printf("coded word: %d\n", codedWordNum);
	//return ;
	//
	fpp = fopen(fileHuffman, "wb");
	fwrite(&wordNumber, sizeof(int), 1, fpp);
	fwrite(wordcodeMap, sizeof(unsigned short), wordNumber, fpp);

	int minwd[2], mincount[2], rootnode;
	int processingtree = codedWordNum;
	while(processingtree > 1)
	{
		minwd[0] = -1;
		mincount[0] = INT_MAX;
		for(i = 0; i < usedTop; i++)
		{
			if(node[i].mark)
				continue;
			if(node[i].weight < mincount[0])
			{
				mincount[0] = node[i].weight;
				minwd[0] = i;
			}
		}
		//
		node[minwd[0]].mark = 1;

		minwd[1] = -1;
		mincount[1] = INT_MAX;
		for(i = 0; i < usedTop; i++)
		{
			if(node[i].mark)
				continue;
			if(node[i].weight < mincount[1])
			{
				mincount[1] = node[i].weight;
				minwd[1] = i;
			}
		}
		node[minwd[1]].mark = 1;
		//
		index = freeSpace[freeTop-1];
		freeTop--;
		assert(freeTop >= 0);
		//
		node[minwd[0]].parent = index;
		node[minwd[1]].parent = index;
		//
		node[index].left = minwd[0];
		node[index].right = minwd[1];
		node[index].weight = mincount[0] + mincount[1];
		node[index].mark = 0;
		node[index].parent = -1;
		rootnode = index;
		usedTop++;
		processingtree--;
//		printf("num of left tree: %d\n", processingtree);
		//
	}
	//遍历，检测压缩后的效果
	long totalstep = 0;
	int step, check=0, nonzero=0;
	int curidx;
	for(i = 0; i < codedWordNum; i++)
	{
		curidx = i;
		step = 0;
		while(node[curidx].parent >= 0)
		{	
			curidx = node[curidx].parent;
			step++;
		}
		totalstep += step * node[i].weight;// / totalcount;
		check+=node[i].weight;
		nonzero++;
	}
	fwrite(&rootnode, sizeof(int), 1, fpp);
	fwrite(node, sizeof(HuffmanNode), usedTop, fpp);
	/*for(i = 0; i < 2*wordNumber-1; i++)
	{
		fwrite(&node[i].left, sizeof(int), 1, fpp);
		fwrite(&node[i].right, sizeof(int), 1, fpp);
	}*/
	fclose(fpp);
	//
	printf("%d, %d, %d, %d, %f\n", nonzero, totalcount, check, totalstep, (double)(totalstep) / check);
	delete node;
	delete usedSpace;
	delete freeSpace;
}

/*************************************************************************************
说明：统计Bigram数据文件vq.bgm中词号的出现频率，并根据词的出现频率由大到小对词重新 
      排序，出现频率最大的词排在最前面。根据新的词号重新组织该数据文件，并按照新词
	  号由大到小重新排序（在W1相同的所有Bigram中排序）。
	  语言模型词号压缩的第一步
Date: May 14, 2001.
Written By: Sheng Gao
***************************************************************************************/
void	ReorderBigram()
{
	int *wordcount, i, wordNumber;
	unsigned short wd;
	FILE *fpp, *fidx;

	//读取Bigram数据文件（经过语言模型概率压缩后的文件）中词号并统计词频
	wordNumber = NO_WORDS;
	wordcount = new int[NO_WORDS];
	memset(wordcount, 0, sizeof(int)*NO_WORDS);
	for(i = 0; i < biGramNum; i++)
	{
		wd = *(unsigned short*)(pBigram + i * 4);
		wordcount[wd]++;
	}

	//根据词号的频率排序，产生新的词号
	unsigned short *newword = new unsigned short[NO_WORDS]; //新词号-->旧词号
	unsigned short *oldword = new unsigned short[NO_WORDS]; //旧词号-->新词号
	//压缩掉出现频率为零的词
	int newct = 0;
	for(i = 0; i < wordNumber; i++)
		if(wordcount[i] > 0)
			newword[newct++] = i;
	
	for(i = 0; i < newct-1; i++)
	{
		int max, maxidx;
		max = wordcount[newword[i]];
		maxidx = i;
		for(int j = i+1; j < newct; j++)
		{
			if(wordcount[newword[j]] > max)
			{
				maxidx = j;
				max = wordcount[newword[j]];
			}
		}

		wd = newword[i];
		newword[i] = newword[maxidx];
		newword[maxidx] = wd;

		int tmpint = wordcount[wd];
		wordcount[wd] = wordcount[newword[maxidx]];
		wordcount[newword[maxidx]] = tmpint;
	}
	memset(oldword, 0xffff, sizeof(unsigned short)*NO_WORDS);
	for(i = 0; i < newct; i++)
		oldword[newword[i]] = i;
		//
	//
	printf("新词号产生完毕\n");
	//
	fpp = fopen("bigram\\bigramWord.map", "wb");
	fwrite(oldword, sizeof(unsigned short), NO_WORDS, fpp);
	fwrite(newword, sizeof(unsigned short), newct, fpp);
	fclose(fpp);

	/*//从词号映射文件中读取
	fpp = fopen("bigram\\bigramWord.map", "rb");
	fseek(fpp, 0, SEEK_END);
	int len = ftell(fpp) / sizeof(unsigned short);
	unsigned short *oldword = new unsigned short[NO_WORDS];
	unsigned short *newword = new unsigned short[len - NO_WORDS];
	fseek(fpp, 0, SEEK_SET);
	fread(oldword, sizeof(unsigned short), NO_WORDS, fpp);
	fread(newword, sizeof(unsigned short), len - NO_WORDS, fpp);
	fclose(fpp);
	*/

	//根据新词号重新组织BIGRAM数据文件，并根据新词号排序
	unsigned short wordlist[NO_WORDS];
	unsigned short *deltaNewword = new unsigned short[NO_WORDS];
	
	if((fpp = fopen("bigram\\bineworder.bgm", "wb")) == NULL) //概率文件
	{
		printf("Error when creating BiNeworder.bgm\n");
		getch();
		return;
	}
	if((fidx = fopen("bigram\\bineworder.bdx", "wb")) == NULL) //索引文件
	{
		printf("Error when creating BiNeworder.bgm\n");
		getch();
		return;
	}
	printf("根据新词号重新排序Bigram\n");
	for(i = 0; i < biGramIdxNum; i++)
	{
		unsigned short num2 = *(unsigned short*)(pBigramIdx + i * 6 + 4);
		long offsetidx = *(long*)(pBigramIdx + i * 6);

		for(int j = 0; j < num2; j++)
		{
			wd = *(unsigned short*)(pBigram + (offsetidx+j) * 4);		
			wordlist[j] = oldword[wd];
		}
		//重新排序
		for(j = 0; j < num2 - 1; j++)
		{
			int minidx;
			unsigned short min = wordlist[j];
			minidx = j;
			for(int k = j+1; k < num2; k++)
			{
				if(wordlist[k] < min)
				{
					minidx = k;
					min = wordlist[k];
				}
			}
			unsigned short tmpint = wordlist[j];
			wordlist[j] = wordlist[minidx];
			wordlist[minidx] = tmpint;
		}
		//计算新词号的增量

		deltaNewword[0] = 0;
		for(j = 1; j < num2; j++)
		{
			deltaNewword[j] = wordlist[j] - wordlist[j-1];
		}
		//
		fwrite(&offsetidx, sizeof(long), 1, fidx);
		fwrite(&num2, sizeof(unsigned short), 1, fidx);
		if(num2 > 0)
			fwrite(&wordlist[0], sizeof(unsigned short), 1, fidx);
		else
		{
			wordlist[0] = 0xffff;
			fwrite(&wordlist[0], sizeof(unsigned short), 1, fidx);
		}
		//
		for(j = 0; j < num2; j++)
		{
			fwrite(&deltaNewword[j], sizeof(unsigned short), 1, fpp);
			fwrite((unsigned char*)(pBigram + (offsetidx+j) * 4+2), sizeof(unsigned char), 1, fpp);
			fwrite((unsigned char*)(pBigram + (offsetidx+j) * 4+3), sizeof(unsigned char), 1, fpp);
		}
	}
	fclose(fpp);
	fclose(fidx);

	delete oldword;
	delete newword;
	delete wordcount;
	delete deltaNewword;
}

/************************************************************
说明：根据新词号和新排序的Bigram数据文件产生的Huffman编码树
*************************************************************/
void	GenerateBigramHuffmanTree()
{
	int *wordcount, i, diff, wordNumber;
	FILE *fpp;

	//读取根据新词号重新排序后的文件
	if((fpp = fopen("bigram\\bineworder.bgm", "rb")) == NULL)
	{
		printf("Bineworder.bgm not found\n");
		getch();
		return;
	}
	fseek(fpp, 0, SEEK_END);
	int len = ftell(fpp);
	unsigned char *pidx=new unsigned char[len];
	fseek(fpp, 0, SEEK_SET);
	fread(pidx, sizeof(unsigned char), len, fpp);
	fclose(fpp);
	//计算词号的增量，并统计增量的出现频率，对增量进行Huffman编码
	wordNumber = NO_WORDS;
	wordcount = new int[NO_WORDS];
	memset(wordcount, 0, sizeof(int) * NO_WORDS);

	for(i = 0; i < biGramNum; i++)
	{
		diff = *(unsigned short*)(pidx + i * 4);		
		wordcount[diff]++;
	}
	printf("开始产生Bigram的Huffman树.....\n");
	GenerateHuffmanTree(wordNumber, wordcount, "bigram\\huffman.dat");
	printf("Huffman树产生成功\n");
	delete wordcount;
	delete pidx;
}

/******************************************************************
说明：对Bigram进行Huffman编码，产生Bigram的三个文件
	  vqprob.bgm：只存概率的码字
	  vqprob.bdx：概率的索引
      wordid.bdx：对应概率的词号索引，查询词号的偏移量，进行Huffman解码
*************************************************************************/
void	BigramHuffmanCode()
{
	int i, totalcount, wordNumber, rootnode, codedWordNum;
	unsigned short wd, *codedwordmap;
	FILE *fpp, *ftr2;

	printf("开始对Bigram词号编码....\n");

	wordNumber = NO_WORDS;
	totalcount = 0;
	codedwordmap = new unsigned short[NO_WORDS];

	if((fpp = fopen("bigram\\huffman.dat", "rb")) == NULL)
	{
		printf("Bigram Huffman Tree not found\n");
		delete codedwordmap;
		return;
	}
	fread(&codedWordNum, sizeof(int), 1, fpp);
	fread(codedwordmap, sizeof(unsigned short), codedWordNum, fpp);
	fread(&rootnode, sizeof(int), 1, fpp);
	
	HuffmanNode *node = new HuffmanNode[rootnode + 1];
	fread(node, sizeof(HuffmanNode), rootnode + 1, fpp);
	fclose(fpp);

	//遍历，检测压缩后的效果
	long totalstep = 0;
	int step, check=0, nonzero=0;
	int curidx;
	for(i = 0; i < codedWordNum; i++)
	{
		if(node[i].right > 0)
			break;

		curidx = i;
		step = 0;
		while(node[curidx].parent >= 0)
		{	
			curidx = node[curidx].parent;
			step++;
		}
		totalstep += step * node[i].weight;// / totalcount;
		check+=node[i].weight;
		nonzero++;
	}

	///从词号映射文件中读取
	fpp = fopen("bigram\\bigramWord.map", "rb");
	fseek(fpp, 0, SEEK_END);
	int len = ftell(fpp) / sizeof(unsigned short);
	unsigned short *oldword = new unsigned short[NO_WORDS];
	unsigned short *newword = new unsigned short[len - NO_WORDS];
	fseek(fpp, 0, SEEK_SET);
	fread(oldword, sizeof(unsigned short), NO_WORDS, fpp);
	fread(newword, sizeof(unsigned short), len - NO_WORDS, fpp);
	fclose(fpp);
	//读取根据新词号重新组织BIGRAM数据文件
	if((fpp = fopen("bigram\\bineworder.bgm", "rb")) == NULL) 
	{
		printf("Error when creating BiNeworder.bgm\n");
		getch();
		return;
	}
	fseek(fpp, 0, SEEK_END);
	int bigramNumTmp = ftell(fpp);
	unsigned char *pbgm = new unsigned char[bigramNumTmp];
	fseek(fpp, 0, SEEK_SET);
	fread(pbgm, sizeof(unsigned char), bigramNumTmp, fpp);
	fclose(fpp);
	bigramNumTmp /= 4;
	///读取根据新词号重新组织BIGRAM数据文件的索引文件
	if((fpp = fopen("bigram\\bineworder.bdx", "rb")) == NULL) 
	{
		printf("Error when creating BiNeworder.bdx\n");
		getch();
		return;
	}
	fseek(fpp, 0, SEEK_END);
	int bigramIdxNumTmp = ftell(fpp);
	unsigned char *pbdx = new unsigned char[bigramIdxNumTmp];
	fseek(fpp, 0, SEEK_SET);
	fread(pbdx, sizeof(unsigned char), bigramIdxNumTmp, fpp);
	fclose(fpp);
	bigramIdxNumTmp /= 8;

	//写入概率
	if((fpp = fopen("bigram\\vqprob.bgm", "wb")) == NULL)
	{
		printf("Create vqprob.bgm\n");
		delete node;
		delete codedwordmap;
		return;
	}
	for(i = 0; i < bigramNumTmp; i++)
	{
		fwrite((unsigned char*)(pbgm + i * 4 + 2), sizeof(unsigned char), 1, fpp);
		fwrite((unsigned char*)(pbgm + i * 4 + 3), sizeof(unsigned char), 1, fpp);
	}
	fclose(fpp);
	//压缩编码
	ftr2 = fopen("bigram\\vqprob.bdx", "wb"); //概率的索引文件
	fpp = fopen("bigram\\wordid.bdx", "wb"); //Huffman压缩后的词号号的索引文件

	unsigned char *code = new unsigned char[totalstep / 8 + 1];
	memset(code, 0, totalstep/8+1);

	long offset = 0, blockoffset;
	offset = 0;
	for(i = 0; i < bigramIdxNumTmp; i++)
	{
		blockoffset = 0;
		int num2 = *(unsigned short*)(pbdx + i * 8 + 4);
		int offsetidx = *(long*)(pbdx + i * 8);
		unsigned short refword = *(unsigned short*)(pbdx + i * 8 + 6);

		for(int j = 0; j < num2; j++)
		{
			//对词号编码
			int bitnum = 0;
			unsigned char codebit[64], tmpch;
			wd = *(unsigned short*)(pbgm + (offsetidx+j) * 4);		
			curidx = codedwordmap[wd];
			HuffmanCoder(curidx, node, codebit, bitnum);
			for(int k = 0; k < bitnum / 2; k++)
			{
				tmpch = codebit[k];
				codebit[k] = codebit[bitnum-1-k];
				codebit[bitnum-1-k] = tmpch;
			}

			unsigned short wdtest = HuffmanDecoder(rootnode, node, codebit);
			if(wdtest != wd)
			{
				printf("编码错误\n");
				getch();
			}

			for(k = 0; k < bitnum; k++)
			{
				long byteCount = (offset+blockoffset) / 8;
				int  bytePos = (offset+blockoffset) % 8;

				code[byteCount] += (codebit[k] << bytePos);
				blockoffset++;
			}
		}
		//
		fwrite((long*)(pbdx + i * 8), sizeof(long), 1, ftr2);
		fwrite((unsigned short*)(pbdx + i * 8+4), sizeof(unsigned short), 1, ftr2);
		fwrite((unsigned short*)(pbdx + i * 8+6), sizeof(unsigned short), 1, ftr2);
		fwrite(&offset, sizeof(long), 1, ftr2);
		//
		offset += blockoffset;
	}
	fwrite(code, sizeof(unsigned char), offset / 8 + 1, fpp);
	fclose(fpp);
	fclose(ftr2);

	printf("Bigram词号编码结束\n");

/////////////////检测编码后的文件是否有错误
	fpp = fopen("bigram\\vqprob.bdx", "rb"); //概率的索引文件
	fseek(fpp, 0, SEEK_END);
	len = ftell(fpp);
	unsigned char *pidxtmp = new unsigned char[len];
	fseek(fpp, 0, SEEK_SET);
	fread(pidxtmp, sizeof(unsigned char), len, fpp);
	fclose(fpp);

	fpp = fopen("bigram\\wordid.bdx", "rb"); //Huffman压缩后的次号的索引文件
	fseek(fpp, 0, SEEK_END);
	len = ftell(fpp);
	unsigned char *codedbit = new unsigned char[len];
	fseek(fpp, 0, SEEK_SET);
	fread(codedbit, sizeof(unsigned char), len, fpp);
	fclose(fpp);

	printf("开始检查编码是否有误....\n");

	for(i = 0; i < bigramIdxNumTmp; i++)
	{
		int num2 = *(unsigned short*)(pidxtmp + i * 12 + 4);
		int offsetidx = *(long*)(pidxtmp + i * 12);
		unsigned short refword = *(unsigned short*)(pidxtmp + i * 12 + 6);
		offset = *(long*)(pidxtmp + i * 12 + 8);

		for(int j = 0; j < num2; j++)
		{
			//对词号编码
			int bitnum = 0;
			wd = *(unsigned short*)(pbgm + (offsetidx+j) * 4);		
			unsigned short wdtest = HuffmanDecoder2(rootnode, node, codedbit, offset);
			if(wdtest != wd)
			{
				printf("编码错误\n");
				getch();
			}
		}
	}
	printf("检查结束，编码无误\n");

	delete pidxtmp;
	delete codedbit;
/////////////////
	delete node;
	delete code;
	delete codedwordmap;
	delete pbdx;
	delete oldword;
	delete newword;
	delete pbgm;
}


/*************************************************************************************
说明：统计Trigram数据文件vq.tgm中词号的出现频率，并根据词的出现频率由大到小对词重新 
      排序，出现频率最大的词排在最前面。根据新的词号重新组织该数据文件，并按照新词
	  号由大到小重新排序（在W1w2相同的所有Bigram中排序）。
	  语言模型词号压缩的第一步
Date: May 14, 2001.
Written By: Sheng Gao
***************************************************************************************/
void	ReorderTrigram()
{
	int *wordcount, i, wordNumber;
	unsigned short wd;
	FILE *fpp, *fidx;

	//读取Bigram数据文件（经过语言模型概率压缩后的文件）中词号并统计词频
	wordNumber = NO_WORDS;
	wordcount = new int[NO_WORDS];
	memset(wordcount, 0, sizeof(int)*NO_WORDS);
	for(i = 0; i < triGramNum; i++)
	{
		wd = *(unsigned short*)(pTrigram + i * 3);
		wordcount[wd]++;
	}
//	for(i = 0; i < trigramIdxNum2; i++)
//	{
//		wd = *(unsigned short*)(pTrigramIdx[1] + i * 3);
//		wordcount[wd]++;
//	}

	//根据词号的频率排序，产生新的词号
	unsigned short *newword = new unsigned short[NO_WORDS]; //新词号-->旧词号
	unsigned short *oldword = new unsigned short[NO_WORDS]; //旧词号-->新词号
	//压缩掉出现频率为零的词
	int newct = 0;
	for(i = 0; i < wordNumber; i++)
		if(wordcount[i] > 0)
			newword[newct++] = i;
	
	for(i = 0; i < newct-1; i++)
	{
		int max, maxidx;
		max = wordcount[newword[i]];
		maxidx = i;
		for(int j = i+1; j < newct; j++)
		{
			if(wordcount[newword[j]] > max)
			{
				maxidx = j;
				max = wordcount[newword[j]];
			}
		}

		wd = newword[i];
		newword[i] = newword[maxidx];
		newword[maxidx] = wd;

		int tmpint = wordcount[wd];
		wordcount[wd] = wordcount[newword[maxidx]];
		wordcount[newword[maxidx]] = tmpint;
	}
	memset(oldword, 0xffff, sizeof(unsigned short)*NO_WORDS);
	for(i = 0; i < newct; i++)
		oldword[newword[i]] = i;
		//
	//
	printf("新词号产生完毕\n");
	//
	fpp = fopen("Trigram\\TrigramWord.map", "wb");
	fwrite(oldword, sizeof(unsigned short), NO_WORDS, fpp);
	fwrite(newword, sizeof(unsigned short), newct, fpp);
	fclose(fpp);
/*
	///从词号映射文件中读取
	fpp = fopen("Trigram\\TrigramWord.map", "rb");
	fseek(fpp, 0, SEEK_END);
	int len = ftell(fpp) / sizeof(unsigned short);
	unsigned short *oldword = new unsigned short[NO_WORDS];
	unsigned short *newword = new unsigned short[len - NO_WORDS];
	fseek(fpp, 0, SEEK_SET);
	fread(oldword, sizeof(unsigned short), NO_WORDS, fpp);
	fread(newword, sizeof(unsigned short), len - NO_WORDS, fpp);
	fclose(fpp);
*/	

	//根据新词号重新组织BIGRAM数据文件，并根据新词号排序
	unsigned short wordlist[NO_WORDS];
	unsigned short *deltaNewword = new unsigned short[NO_WORDS];
	
	if((fpp = fopen("Trigram\\Trineworder.tgm", "wb")) == NULL) //概率文件
	{
		printf("Error when creating BiNeworder.bgm\n");
		getch();
		return;
	}
	if((fidx = fopen("Trigram\\trineworder.tdx", "wb")) == NULL) //索引文件
	{
		printf("Error when creating TriNeworder.tdx\n");
		getch();
		return;
	}
	printf("根据新词号重新排序Trigram\n");
	for(i = 0; i < trigramIdxNum2; i++)
	{
		if(i != trigramIdxNum2 - 1)
		{
			int	offsetidx1 = FindTrigramIdxOffset(i, pTrigramIdxOffset, trigramOffsetNum);
			int	offsetidx2 = FindTrigramIdxOffset(i + 1, pTrigramIdxOffset, trigramOffsetNum);
			
			offsetidx1 = ((offsetidx1 - 1) << 8) + (*(unsigned char*)(pTrigramIdx[1] + i * 3 + 2));
			offsetidx2 = ((offsetidx2 - 1) << 8) + (*(unsigned char*)(pTrigramIdx[1] + (i+1) * 3 + 2));
			int num2 = offsetidx2 - offsetidx1;

			for(int j = 0; j < num2; j++)
			{
				wd = *(unsigned short*)(pTrigram + (offsetidx1+j) * 3);		
				wordlist[j] = oldword[wd];
			}
			//重新排序
			for(j = 0; j < num2 - 1; j++)
			{
				int minidx;
				unsigned short min = wordlist[j];
				minidx = j;
				for(int k = j+1; k < num2; k++)
				{
					if(wordlist[k] < min)
					{
						minidx = k;
						min = wordlist[k];
					}
				}
				unsigned short tmpint = wordlist[j];
				wordlist[j] = wordlist[minidx];
				wordlist[minidx] = tmpint;
			}
			//计算新词号的增量
			deltaNewword[0] = 0;
			for(j = 1; j < num2; j++)
			{
				deltaNewword[j] = wordlist[j] - wordlist[j-1];
			}
			//
			assert(num2 > 0);
			fwrite((unsigned short*)(pTrigramIdx[1] + i * 3), sizeof(unsigned short), 1, fidx);
			fwrite((unsigned char*)(pTrigramIdx[1] + i * 3+2), sizeof(unsigned char), 1, fidx);
			fwrite(&wordlist[0], sizeof(unsigned short), 1, fidx);
			//
			for(j = 0; j < num2; j++)
			{
				fwrite(&deltaNewword[j], sizeof(unsigned short), 1, fpp);
				fwrite((unsigned char*)(pTrigram + (offsetidx1+j) * 3+2), sizeof(unsigned char), 1, fpp);
			}
		}
		else
		{
			fwrite((unsigned short*)(pTrigramIdx[1] + i * 3), sizeof(unsigned short), 1, fidx);
			fwrite((unsigned char*)(pTrigramIdx[1] + i * 3+2), sizeof(unsigned char), 1, fidx);
			wordlist[0] = 0xffff;
			fwrite(&wordlist[0], sizeof(unsigned short), 1, fidx);
		}
	}
	fclose(fpp);
	fclose(fidx);

	delete oldword;
	delete newword;
	delete wordcount;
	delete deltaNewword;
}


/******************************************************************
说明：对Trigram进行Huffman编码，产生Trigram的三个文件
	  vqprob.tgm：只存概率的码字
	  vqprob.tdx：概率的索引
      wordid.tdx：对应概率的词号索引，查询词号的偏移量，进行Huffman解码
*************************************************************************/
void	TrigramHuffmanCode()
{
	int i, totalcount, wordNumber, rootnode, codedWordNum;
	unsigned short wd, *codedwordmap;
	FILE *fpp, *ftr2;

	printf("开始对Trigram词号编码....\n");

	wordNumber = NO_WORDS;
	totalcount = 0;
	codedwordmap = new unsigned short[NO_WORDS];

	if((fpp = fopen("Trigram\\huffman.dat", "rb")) == NULL)
	{
		printf("Trigram Huffman Tree not found\n");
		delete codedwordmap;
		return;
	}
	fread(&codedWordNum, sizeof(int), 1, fpp);
	fread(codedwordmap, sizeof(unsigned short), codedWordNum, fpp);
	fread(&rootnode, sizeof(int), 1, fpp);
	
	//
	HuffmanNode *node = new HuffmanNode[rootnode + 1];
	fread(node, sizeof(HuffmanNode), rootnode + 1, fpp);
	fclose(fpp);

	//遍历，检测压缩后的效果
	long totalstep = 0;
	int step, check=0, nonzero=0;
	int curidx;

	for(i = 0; i < codedWordNum; i++)
	{
		if(node[i].right > 0)
			break;

		curidx = i;
		step = 0;
		while(node[curidx].parent >= 0)
		{	
			curidx = node[curidx].parent;
			step++;
		}
		totalstep += step * node[i].weight;// / totalcount;
		check+=node[i].weight;
		nonzero++;
	}

	///从词号映射文件中读取
	fpp = fopen("Trigram\\TrigramWord.map", "rb");
	fseek(fpp, 0, SEEK_END);
	int len = ftell(fpp) / sizeof(unsigned short);
	unsigned short *oldword = new unsigned short[NO_WORDS];
	unsigned short *newword = new unsigned short[len - NO_WORDS];
	fseek(fpp, 0, SEEK_SET);
	fread(oldword, sizeof(unsigned short), NO_WORDS, fpp);
	fread(newword, sizeof(unsigned short), len - NO_WORDS, fpp);
	fclose(fpp);
	//读取根据新词号重新组织TrIGRAM数据文件
	if((fpp = fopen("Trigram\\Trineworder.tgm", "rb")) == NULL) 
	{
		printf("Error when reading Trineworder.tgm\n");
		getch();
		return;
	}
	fseek(fpp, 0, SEEK_END);
	int trigramNumTmp = ftell(fpp);
	unsigned char *pbgm = new unsigned char[trigramNumTmp];
	fseek(fpp, 0, SEEK_SET);
	fread(pbgm, sizeof(unsigned char), trigramNumTmp, fpp);
	fclose(fpp);
	trigramNumTmp /= 3;
	///读取根据新词号重新组织TRIGRAM数据文件的索引文件
	if((fpp = fopen("trigram\\trineworder.tdx", "rb")) == NULL) 
	{
		printf("Error when reading BiNeworder.tdx\n");
		getch();
		return;
	}
	fseek(fpp, 0, SEEK_END);
	int trigramIdxNumTmp = ftell(fpp);
	unsigned char *pbdx = new unsigned char[trigramIdxNumTmp];
	fseek(fpp, 0, SEEK_SET);
	fread(pbdx, sizeof(unsigned char), trigramIdxNumTmp, fpp);
	fclose(fpp);
	trigramIdxNumTmp /= 5;

	//写入概率
	if((fpp = fopen("trigram\\vqprob.tgm", "wb")) == NULL)
	{
		printf("Create vqprob.bgm\n");
		delete node;
		delete codedwordmap;
		return;
	}
	for(i = 0; i < trigramNumTmp; i++)
	{
		fwrite((unsigned char*)(pbgm + i * 3 + 2), sizeof(unsigned char), 1, fpp);
	}
	fclose(fpp);
	//压缩编码
	ftr2 = fopen("trigram\\vqprob.tdx", "wb"); //概率的索引文件
	fpp = fopen("trigram\\wordid.tdx", "wb"); //Huffman压缩后的词号号的索引文件

	unsigned char *code = new unsigned char[totalstep / 8 + 1];
	memset(code, 0, totalstep/8+1);

	long offset, blockoffset;
	offset = 0;
	//
	for(i = 0; i < trigramIdxNumTmp; i++)
	{
		blockoffset = 0;
		if(i != trigramIdxNumTmp - 1)
		{
			int	offsetidx1 = FindTrigramIdxOffset(i, pTrigramIdxOffset, trigramOffsetNum);
			int	offsetidx2 = FindTrigramIdxOffset(i + 1, pTrigramIdxOffset, trigramOffsetNum);
			
			offsetidx1 = ((offsetidx1 - 1) << 8) + (*(unsigned char*)(pbdx + i * 5 + 2));
			offsetidx2 = ((offsetidx2 - 1) << 8) + (*(unsigned char*)(pbdx + (i+1) * 5 + 2));
			int num2 = offsetidx2 - offsetidx1;

			for(int j = 0; j < num2; j++)
			{
				//对词号编码
				int bitnum = 0;
				unsigned char codebit[64], tmpch;
				wd = *(unsigned short*)(pbgm + (offsetidx1+j) * 3);		

				curidx = codedwordmap[wd];
				HuffmanCoder(curidx, node, codebit, bitnum);

				for(int k = 0; k < bitnum / 2; k++)
				{
					tmpch = codebit[k];
					codebit[k] = codebit[bitnum-1-k];
					codebit[bitnum-1-k] = tmpch;
				}

				unsigned short wdtest = HuffmanDecoder(rootnode, node, codebit);
				if(wdtest != wd)
				{
					printf("编码错误\n");
					getch();
				}
	
				for(k = 0; k < bitnum; k++)
				{
					long byteCount = (offset+blockoffset) / 8;
					int  bytePos = (offset+blockoffset) % 8;

					code[byteCount] += (codebit[k] << bytePos);
					blockoffset++;
				}

			}
			//
			fwrite((unsigned short*)(pbdx + i * 5), sizeof(unsigned short), 1, ftr2);
			fwrite((unsigned char*)(pbdx + i * 5 + 2), sizeof(unsigned char), 1, ftr2);
			fwrite((unsigned short*)(pbdx + i * 5+3), sizeof(unsigned short), 1, ftr2);
			fwrite(&offset, sizeof(long), 1, ftr2);
			offset += blockoffset;
		}
		else
		{
			//
			fwrite((unsigned short*)(pbdx + i * 5), sizeof(unsigned short), 1, ftr2);
			fwrite((unsigned char*)(pbdx + i * 5 + 2), sizeof(unsigned char), 1, ftr2);
			fwrite((unsigned short*)(pbdx + i * 5+3), sizeof(unsigned short), 1, ftr2);
			fwrite(&offset, sizeof(long), 1, ftr2);
			//
		}
	}

	fwrite(code, sizeof(unsigned char), offset / 8 + 1, fpp);
	fclose(fpp);
	fclose(ftr2);

	printf("Trigram词号编码结束\n");
	/////////////////检测编码后的文件是否有错误
	fpp = fopen("trigram\\vqprob.tdx", "rb"); //概率的索引文件
	fseek(fpp, 0, SEEK_END);
	len = ftell(fpp);
	unsigned char *pidxtmp = new unsigned char[len];
	fseek(fpp, 0, SEEK_SET);
	fread(pidxtmp, sizeof(unsigned char), len, fpp);
	fclose(fpp);

	fpp = fopen("trigram\\wordid.tdx", "rb"); //Huffman压缩后的词号的索引文件
	fseek(fpp, 0, SEEK_END);
	len = ftell(fpp);
	unsigned char *codedbit = new unsigned char[len];
	fseek(fpp, 0, SEEK_SET);
	fread(codedbit, sizeof(unsigned char), len, fpp);
	fclose(fpp);
	

	printf("开始检查编码是否有误....\n");
	
	for(i = 0; i < trigramIdxNumTmp; i++)
	{
		blockoffset = 0;
		if(i != trigramIdxNumTmp - 1)
		{
			int	offsetidx1 = FindTrigramIdxOffset(i, pTrigramIdxOffset, trigramOffsetNum);
			int	offsetidx2 = FindTrigramIdxOffset(i + 1, pTrigramIdxOffset, trigramOffsetNum);
			
			offsetidx1 = ((offsetidx1 - 1) << 8) + (*(unsigned char*)(pidxtmp + i * 9 + 2));
			offsetidx2 = ((offsetidx2 - 1) << 8) + (*(unsigned char*)(pidxtmp + (i+1) * 9 + 2));
			int num2 = offsetidx2 - offsetidx1;
			offset = (*(long*)(pidxtmp + i * 9 + 5));

			for(int j = 0; j < num2; j++)
			{
				//对词号编码
				int bitnum = 0;
				wd = *(unsigned short*)(pbgm + (offsetidx1+j) * 3);		
				unsigned short wdtest = HuffmanDecoder2(rootnode, node, codedbit, offset);
				if(wdtest != wd)
				{
					printf("编码错误\n");
					getch();
				}
			}
		}
	}

	printf("检查结束，编码无误\n");

	delete pidxtmp;
	delete codedbit;
/////////////////
	delete node;
	delete code;
	delete codedwordmap;
	delete pbdx;
	delete oldword;
	delete newword;
	delete pbgm;
}

void	GenerateTrigramHuffmanTree()
{
	int *wordcount, i, totalcount = 0;
	unsigned short wd;
	FILE *fpp;
	//
	//读取根据新词号重新排序后的文件
	if((fpp = fopen("Trigram\\Trineworder.tgm", "rb")) == NULL)
	{
		printf("Trineworder.tgm not found\n");
		getch();
		return;
	}
	fseek(fpp, 0, SEEK_END);
	int len = ftell(fpp);
	unsigned char *pidx = new unsigned char[len];
	fseek(fpp, 0, SEEK_SET);
	fread(pidx, sizeof(unsigned char), len, fpp);
	fclose(fpp);
	//计算词号的增量，并统计增量的出现频率，对增量进行Huffman编码
	int wordNumber = NO_WORDS;
	wordcount = new int[NO_WORDS];
	memset(wordcount, 0, sizeof(int)*NO_WORDS);

	/*for(i = 0; i < trigramIdxNum2-1; i++)
	{
		wd = *(unsigned short*)(pTrigramIdx[1] + i * 3);
		wordcount[wd]++;
	}
*/
	for(i = 0; i < triGramNum; i++)
	{
		wd = *(unsigned short*)(pidx + i * 3);
		wordcount[wd]++;
	}
	printf("开始产生Trigram的Huffman树.....\n");
	GenerateHuffmanTree(wordNumber, wordcount, "trigram\\huffman.dat");
	printf("Huffman树产生成功\n");
	delete wordcount;
	delete pidx;
}

////////////采用和Bigram同样的映射
/*************************************************************************************
说明：统计Trigram数据文件vq.tgm中词号的出现频率，并根据词的出现频率由大到小对词重新 
      排序，出现频率最大的词排在最前面。根据新的词号重新组织该数据文件，并按照新词
	  号由大到小重新排序（在W1w2相同的所有Bigram中排序）。
	  语言模型词号压缩的第一步
Date: May 14, 2001.
Written By: Sheng Gao
***************************************************************************************/
void	ReorderTrigram2()
{
	int *wordcount, i, wordNumber;
	unsigned short wd;
	FILE *fpp, *fidx;

	//读取Bigram数据文件（经过语言模型概率压缩后的文件）中词号并统计词频
	wordNumber = NO_WORDS;
	wordcount = new int[NO_WORDS];
	memset(wordcount, 0, sizeof(int)*NO_WORDS);
	for(i = 0; i < triGramNum; i++)
	{
		wd = *(unsigned short*)(pTrigram + i * 3);
		wordcount[wd]++;
	}

	///从词号映射文件中读取
	fpp = fopen("Bigram\\BigramWord.map", "rb");
	fseek(fpp, 0, SEEK_END);
	int len = ftell(fpp) / sizeof(unsigned short);
	unsigned short *oldword = new unsigned short[NO_WORDS];
	unsigned short *newword = new unsigned short[len - NO_WORDS];
	fseek(fpp, 0, SEEK_SET);
	fread(oldword, sizeof(unsigned short), NO_WORDS, fpp);
	fread(newword, sizeof(unsigned short), len - NO_WORDS, fpp);
	fclose(fpp);
	for(i = 0; i < NO_WORDS; i++)
	{
		if(wordcount[i] != 0)
		{
			if(oldword[i] >= len - NO_WORDS)
			{
				printf("Trigram的W3词号不是Bigram W2词号的子集\n");
				delete wordcount;
				delete oldword;
				delete newword;
				getch();
				return;
			}
		}
	}

	//根据新词号重新组织BIGRAM数据文件，并根据新词号排序
	unsigned short wordlist[NO_WORDS];
	unsigned short *deltaNewword = new unsigned short[NO_WORDS];
	
	if((fpp = fopen("Trigram\\Trineworder.tgm", "wb")) == NULL) //概率文件
	{
		printf("Error when creating BiNeworder.bgm\n");
		getch();
		return;
	}
	if((fidx = fopen("Trigram\\trineworder.tdx", "wb")) == NULL) //索引文件
	{
		printf("Error when creating TriNeworder.tdx\n");
		getch();
		return;
	}
	printf("根据新词号重新排序Trigram\n");
	for(i = 0; i < trigramIdxNum2; i++)
	{
		if(i != trigramIdxNum2 - 1)
		{
			int	offsetidx1 = FindTrigramIdxOffset(i, pTrigramIdxOffset, trigramOffsetNum);
			int	offsetidx2 = FindTrigramIdxOffset(i + 1, pTrigramIdxOffset, trigramOffsetNum);
			
			offsetidx1 = ((offsetidx1 - 1) << 8) + (*(unsigned char*)(pTrigramIdx[1] + i * 3 + 2));
			offsetidx2 = ((offsetidx2 - 1) << 8) + (*(unsigned char*)(pTrigramIdx[1] + (i+1) * 3 + 2));
			int num2 = offsetidx2 - offsetidx1;

			for(int j = 0; j < num2; j++)
			{
				wd = *(unsigned short*)(pTrigram + (offsetidx1+j) * 3);		
				wordlist[j] = oldword[wd];
			}
			//重新排序
			for(j = 0; j < num2 - 1; j++)
			{
				int minidx;
				unsigned short min = wordlist[j];
				minidx = j;
				for(int k = j+1; k < num2; k++)
				{
					if(wordlist[k] < min)
					{
						minidx = k;
						min = wordlist[k];
					}
				}
				unsigned short tmpint = wordlist[j];
				wordlist[j] = wordlist[minidx];
				wordlist[minidx] = tmpint;
			}
			//计算新词号的增量
			deltaNewword[0] = 0;
			for(j = 1; j < num2; j++)
			{
				deltaNewword[j] = wordlist[j] - wordlist[j-1];
			}
			//
			assert(num2 > 0);
			fwrite((unsigned short*)(pTrigramIdx[1] + i * 3), sizeof(unsigned short), 1, fidx);
			fwrite((unsigned char*)(pTrigramIdx[1] + i * 3+2), sizeof(unsigned char), 1, fidx);
			fwrite(&wordlist[0], sizeof(unsigned short), 1, fidx);
			//
			for(j = 0; j < num2; j++)
			{
				fwrite(&deltaNewword[j], sizeof(unsigned short), 1, fpp);
				fwrite((unsigned char*)(pTrigram + (offsetidx1+j) * 3+2), sizeof(unsigned char), 1, fpp);
			}
		}
		else
		{
			fwrite((unsigned short*)(pTrigramIdx[1] + i * 3), sizeof(unsigned short), 1, fidx);
			fwrite((unsigned char*)(pTrigramIdx[1] + i * 3+2), sizeof(unsigned char), 1, fidx);
			wordlist[0] = 0xffff;
			fwrite(&wordlist[0], sizeof(unsigned short), 1, fidx);
		}
	}
	fclose(fpp);
	fclose(fidx);

	delete oldword;
	delete newword;
	delete wordcount;
	delete deltaNewword;
}


/******************************************************************
说明：对Trigram进行Huffman编码，产生Trigram的三个文件
	  vqprob.tgm：只存概率的码字
	  vqprob.tdx：概率的索引
      wordid.tdx：对应概率的词号索引，查询词号的偏移量，进行Huffman解码
*************************************************************************/
void	TrigramHuffmanCode2()
{
	int *wordcount, i, totalcount, wordNumber, rootnode, codedWordNum;
	unsigned short wd, *codedwordmap;
	FILE *fpp, *ftr2;

	printf("开始对Trigram词号编码....\n");

	wordNumber = NO_WORDS;
	totalcount = 0;
	codedwordmap = new unsigned short[NO_WORDS];

	if((fpp = fopen("Trigram\\huffman.dat", "rb")) == NULL)
	{
		printf("Trigram Huffman Tree not found\n");
		delete codedwordmap;
		return;
	}
	fread(&codedWordNum, sizeof(int), 1, fpp);
	fread(codedwordmap, sizeof(unsigned short), codedWordNum, fpp);
	fread(&rootnode, sizeof(int), 1, fpp);
	
	//
	HuffmanNode *node = new HuffmanNode[rootnode + 1];
	fread(node, sizeof(HuffmanNode), rootnode + 1, fpp);
	fclose(fpp);

	///从词号映射文件中读取
	fpp = fopen("bigram\\bigramWord.map", "rb");
	fseek(fpp, 0, SEEK_END);
	int len = ftell(fpp) / sizeof(unsigned short);
	unsigned short *oldword = new unsigned short[NO_WORDS];
	unsigned short *newword = new unsigned short[len - NO_WORDS];
	fseek(fpp, 0, SEEK_SET);
	fread(oldword, sizeof(unsigned short), NO_WORDS, fpp);
	fread(newword, sizeof(unsigned short), len - NO_WORDS, fpp);
	fclose(fpp);
	//读取根据新词号重新组织TrIGRAM数据文件
	if((fpp = fopen("Trigram\\Trineworder.tgm", "rb")) == NULL) 
	{
		printf("Error when reading Trineworder.tgm\n");
		getch();
		return;
	}
	fseek(fpp, 0, SEEK_END);
	int trigramNumTmp = ftell(fpp);
	unsigned char *pbgm = new unsigned char[trigramNumTmp];
	fseek(fpp, 0, SEEK_SET);
	fread(pbgm, sizeof(unsigned char), trigramNumTmp, fpp);
	fclose(fpp);
	trigramNumTmp /= 3;
	//
	//计算词号的增量，并统计增量的出现频率，对增量进行Huffman编码
	wordcount = new int[NO_WORDS];
	memset(wordcount, 0, sizeof(int)*NO_WORDS);
	for(i = 0; i < trigramNumTmp; i++)
	{
		wd = *(unsigned short*)(pbgm + i * 3);
		wordcount[wd]++;
	}
	//遍历，检测压缩后的效果
	long totalstep = 0;
	int step, check=0, nonzero=0;
	int curidx;

	for(i = 0; i < codedWordNum; i++)
	{
		if(node[i].right > 0)
			break;

		curidx = i;
		step = 0;
		while(node[curidx].parent >= 0)
		{	
			curidx = node[curidx].parent;
			step++;
		}
		totalstep += step * wordcount[node[i].left];// / totalcount;
		check+=wordcount[node[i].left];
		nonzero++;
	}



	///读取根据新词号重新组织TRIGRAM数据文件的索引文件
	if((fpp = fopen("trigram\\trineworder.tdx", "rb")) == NULL) 
	{
		printf("Error when reading BiNeworder.tdx\n");
		getch();
		return;
	}
	fseek(fpp, 0, SEEK_END);
	int trigramIdxNumTmp = ftell(fpp);
	unsigned char *pbdx = new unsigned char[trigramIdxNumTmp];
	fseek(fpp, 0, SEEK_SET);
	fread(pbdx, sizeof(unsigned char), trigramIdxNumTmp, fpp);
	fclose(fpp);
	trigramIdxNumTmp /= 5;

	//写入概率
	if((fpp = fopen("trigram\\vqprob.tgm", "wb")) == NULL)
	{
		printf("Create vqprob.bgm\n");
		delete node;
		delete codedwordmap;
		delete wordcount;
		return;
	}
	for(i = 0; i < trigramNumTmp; i++)
	{
		fwrite((unsigned char*)(pbgm + i * 3 + 2), sizeof(unsigned char), 1, fpp);
	}
	fclose(fpp);
	//压缩编码
	ftr2 = fopen("trigram\\vqprob.tdx", "wb"); //概率的索引文件
	fpp = fopen("trigram\\wordid.tdx", "wb"); //Huffman压缩后的词号号的索引文件

	unsigned char *code = new unsigned char[totalstep / 8 + 1];
	memset(code, 0, totalstep/8+1);

	long offset, blockoffset;
	offset = 0;
	//
	for(i = 0; i < trigramIdxNumTmp; i++)
	{
		blockoffset = 0;
		if(i != trigramIdxNumTmp - 1)
		{
			int	offsetidx1 = FindTrigramIdxOffset(i, pTrigramIdxOffset, trigramOffsetNum);
			int	offsetidx2 = FindTrigramIdxOffset(i + 1, pTrigramIdxOffset, trigramOffsetNum);
			
			offsetidx1 = ((offsetidx1 - 1) << 8) + (*(unsigned char*)(pbdx + i * 5 + 2));
			offsetidx2 = ((offsetidx2 - 1) << 8) + (*(unsigned char*)(pbdx + (i+1) * 5 + 2));
			int num2 = offsetidx2 - offsetidx1;

			for(int j = 0; j < num2; j++)
			{
				//对词号编码
				int bitnum = 0;
				unsigned char codebit[64], tmpch;
				wd = *(unsigned short*)(pbgm + (offsetidx1+j) * 3);		

				curidx = codedwordmap[wd];
				HuffmanCoder(curidx, node, codebit, bitnum);

				for(int k = 0; k < bitnum / 2; k++)
				{
					tmpch = codebit[k];
					codebit[k] = codebit[bitnum-1-k];
					codebit[bitnum-1-k] = tmpch;
				}

				unsigned short wdtest = HuffmanDecoder(rootnode, node, codebit);
				if(wdtest != wd)
				{
					printf("编码错误\n");
					getch();
				}
	
				for(k = 0; k < bitnum; k++)
				{
					long byteCount = (offset+blockoffset) / 8;
					int  bytePos = (offset+blockoffset) % 8;

					code[byteCount] += (codebit[k] << bytePos);
					blockoffset++;
				}

			}
			//
			fwrite((unsigned short*)(pbdx + i * 5), sizeof(unsigned short), 1, ftr2);
			fwrite((unsigned char*)(pbdx + i * 5 + 2), sizeof(unsigned char), 1, ftr2);
			fwrite((unsigned short*)(pbdx + i * 5+3), sizeof(unsigned short), 1, ftr2);
			fwrite(&offset, sizeof(long), 1, ftr2);
			offset += blockoffset;
		}
		else
		{
			//
			fwrite((unsigned short*)(pbdx + i * 5), sizeof(unsigned short), 1, ftr2);
			fwrite((unsigned char*)(pbdx + i * 5 + 2), sizeof(unsigned char), 1, ftr2);
			fwrite((unsigned short*)(pbdx + i * 5+3), sizeof(unsigned short), 1, ftr2);
			fwrite(&offset, sizeof(long), 1, ftr2);
			//
		}
	}

	fwrite(code, sizeof(unsigned char), offset / 8 + 1, fpp);
	fclose(fpp);
	fclose(ftr2);

	printf("Trigram词号编码结束\n");
	/////////////////检测编码后的文件是否有错误
	fpp = fopen("trigram\\vqprob.tdx", "rb"); //概率的索引文件
	fseek(fpp, 0, SEEK_END);
	len = ftell(fpp);
	unsigned char *pidxtmp = new unsigned char[len];
	fseek(fpp, 0, SEEK_SET);
	fread(pidxtmp, sizeof(unsigned char), len, fpp);
	fclose(fpp);

	fpp = fopen("trigram\\wordid.tdx", "rb"); //Huffman压缩后的词号的索引文件
	fseek(fpp, 0, SEEK_END);
	len = ftell(fpp);
	unsigned char *codedbit = new unsigned char[len];
	fseek(fpp, 0, SEEK_SET);
	fread(codedbit, sizeof(unsigned char), len, fpp);
	fclose(fpp);
	

	printf("开始检查编码是否有误....\n");
	
	for(i = 0; i < trigramIdxNumTmp; i++)
	{
		blockoffset = 0;
		if(i != trigramIdxNumTmp - 1)
		{
			int	offsetidx1 = FindTrigramIdxOffset(i, pTrigramIdxOffset, trigramOffsetNum);
			int	offsetidx2 = FindTrigramIdxOffset(i + 1, pTrigramIdxOffset, trigramOffsetNum);
			
			offsetidx1 = ((offsetidx1 - 1) << 8) + (*(unsigned char*)(pidxtmp + i * 9 + 2));
			offsetidx2 = ((offsetidx2 - 1) << 8) + (*(unsigned char*)(pidxtmp + (i+1) * 9 + 2));
			int num2 = offsetidx2 - offsetidx1;
			offset = (*(long*)(pidxtmp + i * 9 + 5));

			for(int j = 0; j < num2; j++)
			{
				//对词号编码
				int bitnum = 0;
				wd = *(unsigned short*)(pbgm + (offsetidx1+j) * 3);		
				unsigned short wdtest = HuffmanDecoder2(rootnode, node, codedbit, offset);
				if(wdtest != wd)
				{
					printf("编码错误\n");
					getch();
				}
			}
		}
	}

	printf("检查结束，编码无误\n");

	delete pidxtmp;
	delete codedbit;
/////////////////
	delete node;
	delete code;
	delete codedwordmap;
	delete pbdx;
	delete oldword;
	delete newword;
	delete pbgm;
	delete wordcount;
}

void	GenerateTrigramHuffmanTree2()
{
	int *wordcount, i, totalcount = 0;
	unsigned short wd;
	FILE *fpp;
	//
	//读取根据新词号重新排序后的文件
	if((fpp = fopen("Trigram\\Trineworder.tgm", "rb")) == NULL)
	{
		printf("Trineworder.tgm not found\n");
		getch();
		return;
	}
	fseek(fpp, 0, SEEK_END);
	int len = ftell(fpp);
	unsigned char *pidx = new unsigned char[len];
	fseek(fpp, 0, SEEK_SET);
	fread(pidx, sizeof(unsigned char), len, fpp);
	fclose(fpp);
	//计算词号的增量，并统计增量的出现频率，对增量进行Huffman编码
	int wordNumber = NO_WORDS;
	wordcount = new int[NO_WORDS];
	memset(wordcount, 0, sizeof(int)*NO_WORDS);

	/*for(i = 0; i < trigramIdxNum2-1; i++)
	{
		wd = *(unsigned short*)(pTrigramIdx[1] + i * 3);
		wordcount[wd]++;
	}
*/
	for(i = 0; i < triGramNum; i++)
	{
		wd = *(unsigned short*)(pidx + i * 3);
		wordcount[wd]++;
	}
	printf("开始产生Trigram的Huffman树.....\n");
	GenerateHuffmanTree(wordNumber, wordcount, "trigram\\huffman.dat");
	printf("Huffman树产生成功\n");
	delete wordcount;
	delete pidx;
}


void	HuffmanCoder(int curidx, HuffmanNode* node, unsigned char *codebit, int &bitnum)
{
	while(node[curidx].parent >= 0)
	{	
		if(node[node[curidx].parent].left == curidx)
			codebit[bitnum] = 0;
		else if(node[node[curidx].parent].right == curidx)
			codebit[bitnum] = 1;
		else
		{
			printf("error when coding\n");
			getch();
		}
		bitnum++;
		//
		curidx = node[curidx].parent;
	}
}

unsigned short	HuffmanDecoder(int rootnode, HuffmanNode* node, unsigned char *codebit)
{
	int curidx = rootnode, pos = 0;
	while(1)
	{
		if(node[curidx].right < 0)
		{
			return node[curidx].left;
			break;
		}
		if(codebit[pos])
			curidx = node[curidx].right;
		else
			curidx = node[curidx].left;
		pos++;
	}
	return 0xffff;
}

unsigned short	HuffmanDecoder2(int rootnode, HuffmanNode* node, unsigned char *codebit, long& offset)
{
	int curidx = rootnode;
	int bytect, bytepos;
	unsigned char tmpch;

	while(1)
	{
		if(node[curidx].right < 0)
		{
			return node[curidx].left;
			break;
		}
		bytect = offset / 8;
		bytepos = offset % 8;
		tmpch = (codebit[bytect] >> bytepos) & 0x1;
		if(tmpch)
			curidx = node[curidx].right;
		else
			curidx = node[curidx].left;
		offset++;
	}
	return 0xffff;
}


void	AnalizeLmProb()
{
	int *wordcount, i, totalcount = 0;
	unsigned short wd;
	int wordNumber = 256;

	///统计出现次数
	wordcount = new int[wordNumber];
	memset(wordcount, 0, sizeof(int)*wordNumber);
	
	for(i = 0; i < triGramNum; i++)
	{
		wd = *(unsigned char*)(pTrigram + i * 3 + 2);
		wordcount[wd]++;
	}
	for(i = 0; i < biGramNum; i++)
	{
		wd = *(unsigned char*)(pBigram + i * 4 + 2);
		wordcount[wd]++;
		wd = *(unsigned char*)(pBigram + i * 4 + 3);
		wordcount[wd]++;
	}
	//产生Huffman编码树
	GenerateHuffmanTree(wordNumber, wordcount, "huffman.dat");
	delete wordcount;
}

//
void ReleaseMemory()
{
	delete pHanzi;
	delete pBigramIdx;
	delete pBigram;
#ifndef LOAD_TRIGRAM_SECOND_INDEX 
	delete pTrigramIdx[0];
#else
	delete pTrigramIdx[0];
	delete pTrigramIdx[1];
#endif
	delete pTrigramIdxOffset;
	delete pTrigram;
}

/*
long CheckTriBackoff(unsigned short w1, unsigned short w2)
{
	long offSet, probLmMax;
	unsigned char   *pTmp;
	int num, low, high, mid;
	unsigned short nCurIdx2;

	// Just check the most favorate one
	offSet = *(long*)(pBigramIdx + w1 * 6L);
	num = *(unsigned short*)(pBigramIdx + w1 * 6L+ 4);

	pTmp = pBigram + offSet * 10L;

	probLmMax = neg_huge;
	if(num > 0)
	{
		nCurIdx2 = *(unsigned short*)(pTmp);
		if(w2 < nCurIdx2)
		{
			if(probLmMax == -19980049)
				probLmMax = neg_huge;
			return probLmMax;
		}
		nCurIdx2 = *(unsigned short*)(pTmp + (num-1)*10);
		if(w2 > nCurIdx2)
		{
			if(probLmMax == -19980049)
				probLmMax = neg_huge;
			return probLmMax;
		}

		//changed by deng yong gang, binary search:
		//modified by zhang hong, because of the change of data structure.
		low = 0;
		high = num-1;

		//find the first idx2
		while( low <= high ) 
		{
			mid = (low + high) / 2;
			nCurIdx2 = *(unsigned short*)(pTmp + mid * 10);

			if (nCurIdx2 == w2)
			{
				probLmMax = *(long*)(pTmp + mid * 10 + 6);
				break;
			}
			else if (nCurIdx2 > w2) 
			{
				high = mid - 1; //continue in low interval
			}
			else 
			{
				low = mid + 1; //continue in high interval
			}
		}// end of while
	}
	if(probLmMax == -19980049)
		probLmMax = neg_huge;

	return probLmMax;
}

long CheckBigram(unsigned short w1, unsigned short w2)
{
	long offSet, probLmMax;
	unsigned char   *pTmp;
	int num, low, high, mid;
	unsigned short nCurIdx2;

	// Just check the most favorate one
	offSet = *(long*)(pBigramIdx + w1 * 6L);
	num = *(unsigned short*)(pBigramIdx + w1 * 6L+ 4);

	pTmp = pBigram + offSet * 10L;

	probLmMax = neg_huge;
	if(num > 0)
	{
		nCurIdx2 = *(unsigned short*)(pTmp);
		if(w2 < nCurIdx2)
		{
			if(probLmMax == -19980049)
				probLmMax = neg_huge;
			return probLmMax;
		}
		nCurIdx2 = *(unsigned short*)(pTmp + (num-1)*10);
		if(w2 > nCurIdx2)
		{
			if(probLmMax == -19980049)
				probLmMax = neg_huge;
			return probLmMax;
		}

		//changed by deng yong gang, binary search:
		//modified by zhang hong, because of the change of data structure.
		low = 0;
		high = num-1;

		//find the first idx2
		while( low <= high ) 
		{
			mid = (low + high) / 2;
			nCurIdx2 = *(unsigned short*)(pTmp + mid * 10);

			if (nCurIdx2 == w2)
			{
				probLmMax = *(long*)(pTmp + mid * 10 + 2);
				break;
			}
			else if (nCurIdx2 > w2) 
			{
				high = mid - 1; //continue in low interval
			}
			else 
			{
				low = mid + 1; //continue in high interval
			}
		}// end of while
	}
	if(probLmMax == -19980049)
		probLmMax = neg_huge;

	return probLmMax;
}

long CheckTrigram(unsigned short w1, unsigned short w2, unsigned short w3)
{
	unsigned char   *pTmp;
	long  offSet;
	int num;
	long word_ptr,Ptri;

	// Just check the most favorate one
	offSet = *(long*)(pTrigramIdx + w1 * 8);
	num = *(unsigned int*)(pTrigramIdx + w1 * 8 + 4);
	pTmp = pTrigram + offSet * 8;
	word_ptr = 0;

	//changed by deng yong gang, binary search:
	//modified by zhang hong, because of the change of data structure.
	int low = 0;
	int high = num-1;
	int mid;
	bool bFound = false;
	//find the first idx2
	Ptri = neg_huge;
	while( low <= high ) 
	{
		mid = (low + high) / 2;
		unsigned short nCurIdx2 = *(unsigned short*)(pTmp + mid * 8);
		unsigned short nCurIdx3 = *(unsigned short*)(pTmp + mid * 8 + 2);//add by zh

		if (nCurIdx2 == w2 && nCurIdx3 == w3)	
		{//edit by zh
			// found here!
			Ptri=*(long *)(pTmp+mid*8+4);
			if(Ptri == -19980049)
				Ptri = neg_huge;
			break;
		}
		else if (nCurIdx2 > w2) 
		{//continue in low interval
			high = mid-1; //continue in low interval
		}
		else if (nCurIdx2 == w2 && nCurIdx3 > w3) 
		{//continue in low interval---- add by zh
			high = mid-1; //continue in low interval---add by zh
		}
		else 
		{
			low = mid + 1; //continue in high interval
		}
	}// end of while

	return Ptri;
}       
*/
void	ClusterLm()
{
	long probTmp, probMin, probMax, gloalMax;
	int i;
	long sampleNum, sampleCt;
	double *pSample;

	sampleNum = triGramNum + biGramNum * 2 + uniGramNum * 2;
	pSample = new double[sampleNum];
	//
	sampleCt = 0;
	probMin = pos_huge;
	probMax = neg_huge;
	gloalMax = neg_huge;

	for(i = 0; i < triGramNum; i++)
	{
#ifndef LOAD_TRIGRAM_SECOND_INDEX
		probTmp = *(long*)(pTrigram + i * 8 + 4);
#else
		probTmp = *(long*)(pTrigram + i * 6 + 2);
#endif

		if(probTmp == NEG_LM)
			continue;
		if(probTmp > probMax)
			probMax = probTmp;
		if(probTmp < probMin)
			probMin = probTmp;
		//
		pSample[sampleCt] = probTmp;
		sampleCt++;
	}
	if(gloalMax < abs(probMax))
		gloalMax = abs(probMax);
	if(gloalMax < abs(probMin))
		gloalMax = abs(probMin);
		
	printf("max prob in trigram: %d\n", probMax);
	printf("min prob in trigram: %d\n", probMin);
	//
	probMin = pos_huge;
	probMax = neg_huge;
	for(i = 0; i < biGramNum; i++)
	{
		probTmp = *(long*)(pBigram + i * 10 + 2);
		if(probTmp == NEG_LM)
			continue;
		if(probTmp > probMax)
			probMax = probTmp;
		if(probTmp < probMin)
			probMin = probTmp;
		//
		pSample[sampleCt] = probTmp;
		sampleCt++;
	}
	if(gloalMax < abs(probMax))
		gloalMax = abs(probMax);
	if(gloalMax < abs(probMin))
		gloalMax = abs(probMin);
	printf("max prob in Bigram: %d\n", probMax);
	printf("min prob in Bigram: %d\n", probMin);
	//
	probMin = pos_huge;
	probMax = neg_huge;
	for(i = 0; i < biGramNum; i++)
	{
		probTmp = *(long*)(pBigram + i * 10 + 6);
		if(probTmp == NEG_LM)
			continue;
		if(probTmp > probMax)
			probMax = probTmp;
		if(probTmp < probMin)
			probMin = probTmp;
		//
		pSample[sampleCt] = probTmp;
		sampleCt++;
	}
	if(gloalMax < abs(probMax))
		gloalMax = abs(probMax);
	if(gloalMax < abs(probMin))
		gloalMax = abs(probMin);
	printf("max prob in Trigram backoff: %d\n", probMax);
	printf("min prob in Trigram backoff: %d\n", probMin);
	//
	probMin = pos_huge;
	probMax = neg_huge;
	for(i = 0; i < uniGramNum; i++)
	{
		probTmp = *(long*)(pUnigram + i * 8);
		if(probTmp == NEG_LM)
			continue;
		if(probTmp > probMax)
			probMax = probTmp;
		if(probTmp < probMin)
			probMin = probTmp;
		//
		pSample[sampleCt] = probTmp;
		sampleCt++;
	}
	if(gloalMax < abs(probMax))
		gloalMax = abs(probMax);
	if(gloalMax < abs(probMin))
		gloalMax = abs(probMin);
	printf("max prob in Unigram: %d\n", probMax);
	printf("min prob in Unigram: %d\n", probMin);
	//
	probMin = pos_huge;
	probMax = neg_huge;
	for(i = 0; i < uniGramNum; i++)
	{
		probTmp = *(long*)(pUnigram + i * 8 + 4);
		if(probTmp == NEG_LM)
			continue;
		if(probTmp > probMax)
			probMax = probTmp;
		if(probTmp < probMin)
			probMin = probTmp;
		//
		pSample[sampleCt] = probTmp;
		sampleCt++;
	}
	if(gloalMax < abs(probMax))
		gloalMax = abs(probMax);
	if(gloalMax < abs(probMin))
		gloalMax = abs(probMin);
	printf("max prob in Bigram backoff: %d\n", probMax);
	printf("min prob in Bigram backoff: %d\n", probMin);
	printf("gloal max range : %d\n", gloalMax);
	//
	if(sampleCt != sampleNum)
	{
//		printf("由异常样本\n");
//		getch();
	}
	//样本归一化
	for(i = 0; i < sampleCt; i++)
		pSample[i] = pSample[i] / (double)gloalMax;
	//对样本进行分类
	BinaryClusterLm(pSample, sampleCt, gloalMax);
	//对样本进行量化

	delete []pSample;
}

void	VqLm()
{
	long probTmp;
	int i, codeSize;
	long prob;
	double sample, *clusterMean;

#ifndef LOAD_TRIGRAM_SECOND_INDEX
	unsigned short vqCode;
#else
	unsigned char vqCode;
#endif
	
	//读取码本
	FILE* fp = fopen("bookLm255.dat", "rb");
	if(fp != NULL)
	{
		fseek(fp, 0L, SEEK_END);
		codeSize = ftell(fp) / sizeof(long);
		clusterMean = new double[codeSize];
		fseek(fp, 0L, SEEK_SET);
		for(i = 0; i < codeSize; i++)
		{
			fread(&prob, sizeof(long), 1, fp);
			clusterMean[i] = (double)(prob) / VQ_REF_VALUE;
		}
		fclose(fp);
	}
	else
		return;

	//量化trigram模型
	if((fp = fopen("vq.tgm", "wb")) == NULL)
	{
		printf("Create file error\n");
		delete clusterMean;
		return;
	}	
	printf("转换trigram\n");
	for(i = 0; i < triGramNum; i++)
	{
#ifndef LOAD_TRIGRAM_SECOND_INDEX
		probTmp = *(long*)(pTrigram + i * 8 + 4);
#else
		probTmp = *(long*)(pTrigram + i * 6 + 2);
#endif

		sample = ((double)probTmp) / VQ_REF_VALUE;
		vqCode = FindCodeword(clusterMean, codeSize, sample);
		if(probTmp == NEG_LM)
		{	
			//vqCode = NEG_LM_NEW;
			assert(vqCode == NEG_LM_BYTE);
		}

#ifndef LOAD_TRIGRAM_SECOND_INDEX
		fwrite(pTrigram + i * 8, sizeof(unsigned char), 4, fp);
#else
		fwrite(pTrigram + i * 6, sizeof(unsigned char), 2, fp);
#endif
		fwrite(&vqCode, sizeof(CODEWORD_SIZE), 1, fp);
	}
	fclose(fp);
	//量化Bigram模型
	printf("转换bigram\n");
	if((fp = fopen("vq.bgm", "wb")) == NULL)
	{
		printf("Create file error\n");
		delete clusterMean;
		return;
	}	
	for(i = 0; i < biGramNum; i++)
	{
		//量化bigram概率
		probTmp = *(long*)(pBigram + i * 10 + 2);
		sample = ((double)probTmp) / VQ_REF_VALUE;
		vqCode = FindCodeword(clusterMean, codeSize, sample);
		if(probTmp == NEG_LM)
		{
			assert(vqCode == NEG_LM_BYTE);
		}

		fwrite(pBigram + i * 10, sizeof(unsigned char), 2, fp);
		fwrite(&vqCode, sizeof(CODEWORD_SIZE), 1, fp);
		//量化trigram backoff概率
		probTmp = *(long*)(pBigram + i * 10 + 6);
		sample = ((double)probTmp) / VQ_REF_VALUE;
		vqCode = FindCodeword(clusterMean, codeSize, sample);
		if(probTmp == NEG_LM)
		{
			assert(vqCode == NEG_LM_BYTE);
			//vqCode = NEG_LM_NEW;
		}
		fwrite(&vqCode, sizeof(CODEWORD_SIZE), 1, fp);
	}
	fclose(fp);
	//量化unigram模型
	if((fp = fopen("vq.lm", "wb")) == NULL)
	{
		printf("Create file error\n");
		delete clusterMean;
		return;
	}	
	printf("转换unigram\n");
	for(i = 0; i < uniGramNum; i++)
	{
		//量化unigram概率
		probTmp = *(long*)(pUnigram + i * 8);
		sample = ((double)probTmp) / VQ_REF_VALUE;
		vqCode = FindCodeword(clusterMean, codeSize, sample);
		if(probTmp == NEG_LM)
		{
			assert(vqCode == NEG_LM_BYTE);
//			vqCode = NEG_LM_NEW;
		}
		fwrite(&vqCode, sizeof(CODEWORD_SIZE), 1, fp);
		//量化bigram backoff 概率
		probTmp = *(long*)(pUnigram + i * 8 +4);
		sample = ((double)probTmp) / VQ_REF_VALUE;
		vqCode = FindCodeword(clusterMean, codeSize, sample);
		if(probTmp == NEG_LM)
		{
			assert(vqCode == NEG_LM_BYTE);
		//	vqCode = NEG_LM_NEW;
		}
		fwrite(&vqCode, sizeof(CODEWORD_SIZE), 1, fp);
	}
	fclose(fp);
	//
	delete clusterMean;
}

unsigned short FindCodeword(double *pMean, int codeSize, double sample)
{
	double dis, diff, mindis;
	int minidx;

	mindis = pos_huge;
	minidx = -1;
	for(int i = 0; i < codeSize; i++)
	{
		diff = pMean[i] - sample;
		dis = diff * diff;
		if(dis < mindis)
		{
			mindis = dis;
			minidx = i;
		}
	}
	assert(minidx >= 0);
	return minidx;
}
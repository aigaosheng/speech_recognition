/*****************************************************************************
文件名：loaddata.cpp
功能描述：装载模型数据
创建日期：Sep.27,2000
作者：高升
*****************************************************************************/
#include	<stdio.h>
#include	<stdlib.h>
#include	<assert.h>
#include	<float.h>
#include	<string.h>
#include	<conio.h>
#include	<process.h>

#include	"include\const.h"
#include	"include\struct.h"
#include	"include\extern.h"

//函数原型声明
int	LoadData(void);
int	LoadUnigram(void);
int LoadBigram(void);
int LoadTrigram(void);
int LoadLexicalTree(void);
unsigned char LoadHmm(void);
int LoadOutput(void);
unsigned char LoadTriGraph(void);
int	LoadShortlist(void);
int LoadLmCodebook(void);
int LoadExpTable(void);

//
int LoadData(void)
{
	if(LoadHmm())
	{
		printf("HMM not found\n");
		getch();
		exit(-1);
	}
	if(LoadOutput())
	{
		printf("Output not found\n");
		getch();
		exit(-1);
	}
//#ifdef FAST_COMPUTE
	if(LoadShortlist())
	{
		printf("short list database is not found\n");
		getch();
		exit(-1);
	}
//#endif

	if(LoadUnigram())
	{
		printf("unigram not found\n");
		getch();
		exit(-1);
	}
	if(LoadBigram())
	{
		printf("bigram not foud\n");
		getch();
		exit(-1);
	}
	if(LoadTrigram())
	{
		printf("trigram fot found\n");
		getch();
		exit(-1);
	}
	if(LoadLexicalTree())
	{
		printf("lexical tree not found\n");
		getch();
		exit(-1);
	}
	if(LoadLmCodebook())
	{
		printf("Lm code book not found\n");
		getch();
		exit(-1);
	}
	//
#ifdef CHECK_EXP_TABLE
	if(LoadExpTable())
	{
		printf("Exp table not found\n");
		getch();
		exit(-1);
	}
#endif	
	return 0;
}

int LoadUnigram(void)
{
	FILE *funigram;
	int i, j;
	char str[MAX_WORD_LENGTH*2+1];

	//funigram = fopen("data\\gs_80_unigram.lm", "rb");
	funigram = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\vq.lm", "rb");
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
	fseek(funigram, 0L, SEEK_SET);
	fread(pUnigram, sizeof(unsigned char), i, funigram);
	fclose(funigram);
	//Load dictionary
	//funigram = fopen("data\\hanzi.han", "rt");
	funigram = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\hanzi.han", "rt");
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

int LoadBigram()
{
	FILE *fbigram;
	long i;

	//Load bigram index
	//fbigram = fopen("data\\vq.bdx", "rb");
	fbigram = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\gs_bkoff_80.bdx", "rb");
	if(fbigram == NULL)
	{
		printf("bigram index file open error\n");
		return 1;
	}
	fseek(fbigram, 0L, SEEK_END);
	i = ftell(fbigram);
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
	//fbigram = fopen("data\\vq.bgm", "rb");
	fbigram = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\vq.bgm", "rb");
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
	fread(pBigram, sizeof(unsigned char), i, fbigram);
	fclose(fbigram);

	/*/
	int max=0, idx;
	for(i = 0; i < NO_WORDS; i++)
	{
		if(*(unsigned short*)(pBigramIdx+i*6+4) > max)
		{
			max = *(unsigned short*)(pBigramIdx+i*6+4);
			idx = i;
		}
	}
	*/
	return 0;
}

int LoadTrigram()
{
	FILE *ftrigram;
	long i;

	//Load trigram index
	ftrigram = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\gs_bkoff_80.tdx", "rb");
	//ftrigram = fopen("data\\vq.tdx", "rb");
	if(ftrigram == NULL)
	{
		printf("trigram index file open error\n");
		return 1;
	}
	fseek(ftrigram, 0L, SEEK_END);
	i = ftell(ftrigram);
	pTrigramIdx = new unsigned char[i];
	if(pTrigramIdx == NULL)
	{
		printf("memory is not sufficient2\n");
		fclose(ftrigram);
		return 2;
	}
	fseek(ftrigram, 0L, SEEK_SET);
	fread(pTrigramIdx, sizeof(unsigned char), i, ftrigram);
	fclose(ftrigram);
	//Load trigram
	ftrigram = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\vq.tgm", "rb");
	//ftrigram = fopen("data\\vq.tgm", "rb");
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
	fread(pTrigram, sizeof(unsigned char), i, ftrigram);
	fclose(ftrigram);

	return 0;
}

//Load Lexical tree
int LoadLexicalTree(void)
{
	int len;
	int i;
	FILE *f1;

	f1 = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\lextree.dat", "rb");
	//f1 = fopen("data\\lextree.dat", "rb");
	if(f1 == NULL)
	{
		printf("open lexical tree error\n");
		getch();
		return 1;
	}

	fseek(f1, 0L, SEEK_END);
	len = ftell(f1) / sizeof(CLexicalTree);
	pLexicalTree = new CLexicalTree[len];
	fseek(f1, 0L, SEEK_SET);
	fread(pLexicalTree, sizeof(CLexicalTree), len, f1);
	fclose(f1);
	//
	int max = 0;
	for(i = 0; i < len; i++)
		if(pLexicalTree[i].linkNum > max)
			max = pLexicalTree[i].linkNum;
	printf("maximal link num in tree: %d\n", max);
	
	f1 = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\triphoneMapI.dat", "rb");
	//f1 = fopen("data\\triphoneList.dat", "rb");
	if(f1 == NULL)
	{
		printf("triphoneList.dat not found\n");
		getch();
		exit(-1);
	}
	fseek(f1, 0, SEEK_END);
	len = ftell(f1) / sizeof(short);
	fseek(f1, 0, SEEK_SET);
	pTriphoneMapInitial = new short[len];
	fread(pTriphoneMapInitial, sizeof(short), len, f1);
	fclose(f1);

	f1 = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\triphoneMapF.dat", "rb");
	//f1 = fopen("data\\triphoneList.dat", "rb");
	if(f1 == NULL)
	{
		printf("triphoneList.dat not found\n");
		getch();
		exit(-1);
	}
	fseek(f1, 0, SEEK_END);
	len = ftell(f1) / sizeof(short);
	fseek(f1, 0, SEEK_SET);
	pTriphoneMapFinal = new short[len];
	fread(pTriphoneMapFinal, sizeof(short), len, f1);
	fclose(f1);
	/*
	for(i = 0; i <= MAX_CODE_INITIAL_PART; i++)
		fread(triphoneMap[i][0][0][0], sizeof(int), TOTAL_PHONES*TOTAL_PHONES, f1);	
	//for(i = MAX_CODE_INITIAL_PART+1; i <= MAX_CODE_FINAL_PART; i++)
	//	fread(triphoneMap[i], sizeof(int), 5*6*6*TOTAL_PHONES*TOTAL_PHONES, f1);	
	fread(triphoneMap+MAX_CODE_INITIAL_PART+1, sizeof(int), 5*6*6*TOTAL_PHONES*TOTAL_PHONES*(MAX_CODE_FINAL_PART-MAX_CODE_INITIAL_PART), f1);	
	fread(triphoneMap[SILENCE_CODE][0][0][0], sizeof(int), TOTAL_PHONES*TOTAL_PHONES, f1);
	fclose(f1);
	*/
	//
	f1 = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\mapping.dat", "rb");
	//f1 = fopen("data\\mapping.dat", "rb");
	if(f1 == NULL)
	{
		printf("triphoneList.dat not found\n");
		getch();
		exit(-1);
	}
	fread(mapping,  sizeof(int), TOTAL_TRIPHONES, f1);
	fclose(f1);

	/*
	f1 = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\pitchPhoneMap.dat", "rb");
	//f1 = fopen("data\\pitchPhoneMap.dat", "rb");
	if(f1 == NULL)
	{
		printf("pitchPhoneMap.dat not found\n");
		getch();
		exit(-1);
	}
	fread(mappingPitch, sizeof(int), TOTAL_PITCH_PHONES, f1);
	fclose(f1);
*/
/*	for(i = 0; i < TOTAL_PHONES; i++)
		mapping[TOTAL_TRIPHONES+i] = i;
*/
	return 0;
}

unsigned char LoadHmm()
{
	int i,n_State,n_Out,n_Trans;
	float aa[20];
	FILE *HmmFile;

	if((HmmFile=fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\modelList.dat", "rb"))==NULL)
	{
		printf("\nCan't open hmmList.dat!");
		return 1;
	}

	for(i=0;i<TRI_NO_HMM;i++)
	{
		fread(&(Hmm[i].No_State),sizeof(int),1,HmmFile);
		fread(&(Hmm[i].No_Out),sizeof(int),1,HmmFile);
		fread(&(Hmm[i].No_Trans),sizeof(int),1,HmmFile);

		n_State=Hmm[i].No_State;
		n_Out=Hmm[i].No_Out;
		n_Trans=Hmm[i].No_Trans;

		Hmm[i].nj=new int[n_State];
		fread(Hmm[i].nj,sizeof(int),n_State,HmmFile);

		Hmm[i].indj=new int[n_State*n_State];
		fread(Hmm[i].indj,sizeof(int),n_State*n_State,HmmFile);

		Hmm[i].aLinks=new int[n_State*n_State];
		fread(Hmm[i].aLinks,sizeof(int),n_State*n_State,HmmFile);

		Hmm[i].bLinks=new int[n_State*n_State];
		fread(Hmm[i].bLinks,sizeof(int),n_State*n_State,HmmFile);

		Hmm[i].A=new long[n_Trans];
		//fread(Hmm[i].A,sizeof(float),n_Trans,HmmFile);
		fread(aa,sizeof(float),n_Trans,HmmFile);
		for(int j = 0; j < n_Trans; j++)
			Hmm[i].A[j] =  (long)(log10(aa[j]) * logbase);

		Hmm[i].OutputID = new int[n_Out];
		if(Hmm[i].OutputID==NULL)
		{
			printf("\nAllocating Hmm[i].OutputID Error!");
			exit(-1);
		}
		fread(Hmm[i].OutputID,sizeof(int),n_Out,HmmFile);
	}

	fclose(HmmFile);
	/*/查询声母最大的输出分布编码
	int max=-1;
	for(i=0;i<1426;i++)
	{
		for(int j=0;j<Hmm[i].No_Out;j++)
			if(Hmm[i].OutputID[j]>max)
				max=Hmm[i].OutputID[j];
	}
	max=2733;
	for(i=1426;i<TRI_NO_HMM;i++)
	{
		for(int j=0;j<Hmm[i].No_Out;j++)
			if(Hmm[i].OutputID[j]<max)
				max=Hmm[i].OutputID[j];
	}
	/*/
	return 0;
}


int LoadOutput()
{
	int	 i, jj;
	float sum;

	FILE *OutputFile;

	OutputFile=fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\OutputList.dat", "rb");
	if(OutputFile==NULL)
	{
		printf("\nCan't open Outputlist.dat\n");
		return 1;
	}

	for(i=0;i<No_Output;i++)
	{	
		Output[i].CoeMix=new float[No_Mix];
		if(Output[i].CoeMix==NULL)
		{
			printf("\nAllocating Output[i].CoeMix Error!");
			exit(-1);
		}
		fread(Output[i].CoeMix,sizeof(float),No_Mix,OutputFile);

		Output[i].Means=new float[No_Mix*No_Dim];
		if(Output[i].Means==NULL)
		{
			printf("\nAllocating Output[i].Means Error!");
			exit(-1);
		}
		fread(Output[i].Means,sizeof(float),No_Mix*No_Dim,OutputFile);

		Output[i].Variance=new float[No_Mix*No_Dim];
		if(Output[i].Variance==NULL)
		{
			printf("\nAllocating Output[i].Variance Error!");
			exit(-1);
		}
		fread(Output[i].Variance,sizeof(float),No_Mix*No_Dim,OutputFile);
//方差预先计算
		for(int j = 0; j < No_Mix; j++)
		{
			sum = 1;
			for(jj = 0; jj < No_Dim; jj++)
				sum *= sqrt(1.0 / (Output[i].Variance[jj+j*No_Dim] * pai2));
			Output[i].CoeMix[j] *= sum;
		}
		for(j = 0; j < No_Mix*No_Dim; j++)
			Output[i].Variance[j] = 0.5 / Output[i].Variance[j];
//
	}
	fclose(OutputFile);
	return 0;
}

//装载short list数据库
int	LoadShortlist()
{
	int	i, len;
	FILE* fp;

	fp = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\shortlist.idx", "rb");
	if(fp == NULL)
		return 1;
	fseek(fp, 0, SEEK_END);
	len = ftell(fp) / 6;
	shortListNum = new short[len];
	shortListOffset = new int[len];
	fseek(fp, 0, SEEK_SET);
	for(i = 0; i < len; i++)
	{
		fread(shortListNum+i, sizeof(short), 1, fp);
		fread(shortListOffset+i, sizeof(int), 1, fp);
	}
	fclose(fp);
	//
	fp = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\shortlist.dat", "rb");
	if(fp == NULL)
		return 1;
	fseek(fp, 0, SEEK_END);
	len = ftell(fp) / sizeof(char);
	shortList = new char[len];
	fseek(fp, 0, SEEK_SET);
	fread(shortList, sizeof(char), len, fp);
	fclose(fp);
	//
	fp = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\codebook.dat", "rb");
	if(fp == NULL)
		return 1;
//	totalMean = new float[No_Dim];
//	totalVar = new float[No_Dim];
	clusterMean = new float[CODE_BOOK_SIZE*No_Dim];
	clusterVar = new float[CODE_BOOK_SIZE*No_Dim];
	//
//	fread(totalMean, sizeof(float), No_Dim, fp);
//	fread(totalVar, sizeof(float), No_Dim, fp);
	for(i = 0; i < CODE_BOOK_SIZE; i++)
	{
		fread(clusterMean+i*No_Dim, sizeof(float), No_Dim, fp);	
		fread(clusterVar+i*No_Dim, sizeof(float), No_Dim, fp);
		//for(j = 0; j < No_Dim; j++)
			//clusterVar[i*No_Dim+j] = 1.0 / sqrt(totalVar[j] * clusterVar[i*No_Dim+j]);
	}
	fclose(fp);
	return 0;
}
//
int LoadLmCodebook(void)
{
	FILE *fp = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\bookLm.dat", "rb");
	if(fp == NULL)
		return 1;
	fseek(fp, 0L, SEEK_END);
	int codeSize = ftell(fp) / sizeof(long);
	pLmCodebook = new long[codeSize];
	fseek(fp, 0L, SEEK_SET);
	fread(pLmCodebook, sizeof(long), codeSize, fp);
	fclose(fp);
	return 0;
}
//
int LoadExpTable(void)
{
	FILE *fp = fopen("E:\\mywork\\decoder\\onepassPitch\\\data\\exp.dat", "rb");
	if(fp == NULL)
		return 1;
	expTable = new double[EXP_TABLE_SIZE];
	fread(expTable, sizeof(double), EXP_TABLE_SIZE, fp);
	fclose(fp);
	return 0;
}
//


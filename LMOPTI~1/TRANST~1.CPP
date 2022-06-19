#include	<stdio.h>
#include	<stdlib.h>
#include	<conio.h>
#include	<assert.h>

#define	TEMPLATE	int
//int		LoadTrigram(void);
void	TransTrigram(unsigned char *pTrigram, unsigned char* pTrigramIdx, int trigramNo);
void	QuickSort(TEMPLATE*, int, int);
void	Swap(int&, int&);
//
void	ProcessTrigram(void);
//
typedef struct
{
	unsigned short num;
	long offset;
} FIRST_INDEX;
typedef struct
{
	unsigned short w2;
	unsigned short num;
	long offset;
} SECONDARY_INDEX;

/*****************************************************
说明：重新表示Trigram的索引，由一级索引变为二级索引
******************************************************/
void	ProcessTrigram()
{
	unsigned char	*pTrigramIdxTmp, *pTrigramTmp;
	long	 trigramNo;

	//装载Trigram
	//Load trigram index
	long i;
	FILE *ftrigram;
	ftrigram = fopen("gs_bkoff_80.tdx", "rb");
	if(ftrigram == NULL)
	{
		printf("trigram index file open error\n");
		return;
	}
	fseek(ftrigram, 0L, SEEK_END);
	i = ftell(ftrigram);
	trigramNo = i / 8;
	pTrigramIdxTmp = new unsigned char[i];
	assert(pTrigramIdxTmp);
	if(pTrigramIdxTmp == NULL)
	{
		printf("memory is not sufficient2\n");
		fclose(ftrigram);
		return;
	}
	fseek(ftrigram, 0L, SEEK_SET);
	fread(pTrigramIdxTmp, sizeof(unsigned char), i, ftrigram);
	fclose(ftrigram);
	//
	ftrigram = fopen("gs_bkoff_80.tgm", "rb");
	if(ftrigram == NULL)
	{
		printf("trigram file open error\n");
		return;
	}
	fseek(ftrigram, 0L, SEEK_END);
	i = ftell(ftrigram);
	pTrigramTmp = new unsigned char[i];
	assert(pTrigramTmp);
	if(pTrigramTmp == NULL)
	{
		printf("memory is not sufficient2\n");
		fclose(ftrigram);
		return;
	}
	fseek(ftrigram, 0L, SEEK_SET);
	fread(pTrigramTmp, sizeof(unsigned char), i, ftrigram);
	fclose(ftrigram);
	//
	TransTrigram(pTrigramTmp, pTrigramIdxTmp, trigramNo);
	delete pTrigramIdxTmp;
	delete pTrigramTmp;
	//
}

/*
int LoadTrigram()
{
	//Load trigram index
	long i;
	FILE *ftrigram;
	ftrigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\dataNoRightpitch\\gs_bkoff_80.tdx", "rb");
	if(ftrigram == NULL)
	{
		printf("trigram index file open error\n");
		return 1;
	}
	fseek(ftrigram, 0L, SEEK_END);
	i = ftell(ftrigram);
	trigramNo = i / 8;
	pTrigramIdx = new unsigned char[i];
	assert(pTrigramIdx);
	if(pTrigramIdx == NULL)
	{
		printf("memory is not sufficient2\n");
		fclose(ftrigram);
		return 2;
	}
	fseek(ftrigram, 0L, SEEK_SET);
	fread(pTrigramIdx, sizeof(unsigned char), i, ftrigram);
	fclose(ftrigram);
	//
	ftrigram = fopen("E:\\mywork\\decoder\\onepassApiPlat\\data\\dataNoRightpitch\\gs_bkoff_80.tgm", "rb");
	if(ftrigram == NULL)
	{
		printf("trigram file open error\n");
		return 1;
	}
	fseek(ftrigram, 0L, SEEK_END);
	i = ftell(ftrigram);
	pTrigram = new unsigned char[i];
	assert(pTrigram);
	if(pTrigram == NULL)
	{
		printf("memory is not sufficient2\n");
		fclose(ftrigram);
		return 2;
	}
	fseek(ftrigram, 0L, SEEK_SET);
	fread(pTrigram, sizeof(unsigned char), i, ftrigram);
	fclose(ftrigram);

	return 0;
}
*/
void	QuickSort(TEMPLATE *A, int low, int high)
{
	TEMPLATE pivot;
	int scanUp, scanDown, mid;
	
	//if the range is not at least two elements, return
	if(high - low <= 0)
		return;
	else
	{
		//if sublist has rwo elements, cpmpare them and exchange their values if necessary
		if(high - low == 1)
		{
			if(A[high] < A[low])
				Swap(A[low], A[high]);
			return;
		}
	}
	//get the mid indexand assign its value to pivot
	mid = (high + low) / 2;
	pivot = A[mid];
	//exchange the pivot and the low end of the range and initialize the indces scanUp and scanDown
	Swap(A[mid], A[low]);
	scanUp = low + 1;
	scanDown = high;
	//manage the indices to locate elements that are in the wrong sublist; stop when scanDown < scanUp
	do
	{
		//move up lower sublist, stop when scanUp enters upper sublist or identifies an element > pivot
		while(scanUp <= scanDown && A[scanUp] <= pivot)
			scanUp++;
		//scan down upper sublist; stop when scanDown locates an element <= pivot; we guarantee a stop at A[low]
		while(pivot < A[scanDown])
			scanDown--;
		//if indices are still in their sublist, then they identify two elements in wrong sublists. exchange them
		if(scanUp < scanDown)
			Swap(A[scanUp], A[scanDown]);
	}
	while(scanUp < scanDown);

	//
	A[low] = A[scanDown];
	A[scanDown] = pivot;

	//
	if(low < scanDown - 1)
		QuickSort(A, low, scanDown-1);
	//
	if(scanDown + 1 < high)
		QuickSort(A, scanDown + 1, high);
}

void Swap(TEMPLATE &a, TEMPLATE &b)
{
	TEMPLATE t;

	t = a;
	a = b;
	b = t;
}


void TransTrigram(unsigned char *pTrigramTmp, unsigned char* pTrigramIdxTmp, int trigramNo)
{
	FILE *fidx1, *fidx2, *fidxold;
	int i, j, k, num;

	fidx1 = fopen("trigram1.idx", "wb");
	fidx2 = fopen("trigram2.idx", "wb");
	fidxold = fopen("trigram.tgm", "wb");
	if(fidx1 == NULL || fidx1 == NULL || fidxold == NULL)
	{
		printf("create file error\n");
		exit(-1);
	}
	//
	SECONDARY_INDEX secondIdx;// = new SECONDARY_INDEX[trigramNo];
	FIRST_INDEX firstIdx;
	long firstOffset = 0, secondOffset = 0, offset;
	for(i = 0; i < trigramNo; i++)
	{
		firstIdx.num = 0;
		firstIdx.offset = 0;

		num = *(unsigned int*)(pTrigramIdxTmp + i * 8 + 4);
		offset = *(long*)(pTrigramIdxTmp + i * 8);
		j = 0;
		while(j < num)
		{
			secondIdx.w2 = *(unsigned short*)(pTrigramTmp + offset * 8 + j * 8);
			secondIdx.num = 0;
			secondIdx.offset = 0;
			for(k = j; k < num; k++)
			{
				if(secondIdx.w2 == (*(unsigned short*)(pTrigramTmp + offset * 8 + k * 8)))
				{
					secondIdx.num++;
					secondIdx.offset++;
					//写语言模型
					fwrite((unsigned short*)(pTrigramTmp + offset*8 + k*8+2), sizeof(unsigned short), 1, fidxold);
					fwrite((long*)(pTrigramTmp + offset*8 + k*8+4), sizeof(long), 1, fidxold);
				}
				else
				{
					break;
				}
			}
			j = k;

			//写二级索引
			fwrite(&(secondIdx.w2), sizeof(unsigned short), 1, fidx2);
			fwrite(&(secondIdx.num), sizeof(unsigned short), 1, fidx2);
			fwrite(&(secondOffset), sizeof(long), 1, fidx2);
			//
			secondOffset += secondIdx.offset;
			//
			firstIdx.num++;
			firstIdx.offset++;
		}
		//写一级索引
		fwrite(&(firstIdx.num), sizeof(unsigned short), 1, fidx1);
		fwrite(&(firstOffset), sizeof(long), 1, fidx1);
		//
		firstOffset += firstIdx.offset;
	//	printf("%d, %d\n", i, firstOffset);
	}
	fclose(fidx1);
	fclose(fidx2);
	fclose(fidxold);
}

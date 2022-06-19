/*********************************************************************************
说明：对语言模型的概率进行量化
**********************************************************************************/
#include	<stdio.h>
#include	<stdlib.h>
#include	<conio.h>
#include	<limits.h>
#include	<string.h>
#include	<math.h>
#include	<assert.h>

//#define		FROM_FILE_INITIAL

#define		CODE_BOOK_SIZE	256//16//048//1024//
#define		pos_huge	INT_MAX
#define		min_huge	INT_MIN

#define		NEG_LM	-19980049
//
#define		SPLIT_PARAMETER	0.02
#define		STOP_THRESHOLD	0.05//0.001//

double	CalculateDistance(double pSample, double pCenter);
void	BinaryClusterLm(double*, long sampleNum, long ref);

void	BinaryClusterLm(double *pSample, long sampleNum, long refVal)
{
	int		i, k,*sumNum, minidx, cycle, minDisIdx;
	double	*sumMean, dis, mindis, maxDis,
			preSumDis, curSumDis, *codeDis,
			ref, relDis;
	int least3, codeSize, maxIdx, maxCluster;
	double *clusterMean;
	char name[100], tmp[20]; 
	long prob;
	FILE *fp;

	sumNum = new int[CODE_BOOK_SIZE];
	codeDis = new double[CODE_BOOK_SIZE];
	clusterMean = new double[CODE_BOOK_SIZE];
	sumMean = new double[CODE_BOOK_SIZE];
	//初始化
	memset(sumNum, 0, sizeof(int)*CODE_BOOK_SIZE);
	memset(sumMean, 0, sizeof(double)*CODE_BOOK_SIZE);
	codeSize = 0;
	for(i = 0; i < sampleNum; i++)
		sumMean[0] += pSample[i];
	clusterMean[codeSize] = sumMean[0] / (double)sampleNum;
	codeSize++;

#ifdef	FROM_FILE_INITIAL
	fp = fopen("bookLm32.dat", "rb");
	if(fp != NULL)
	{
		fseek(fp, 0L, SEEK_END);
		codeSize = ftell(fp) / sizeof(long);
		fseek(fp, 0L, SEEK_SET);
		for(i = 0; i < codeSize; i++)
		{
			fread(&prob, sizeof(long), 1, fp);
			clusterMean[i] = (double)(prob) / refVal;
		}
		fclose(fp);
	}
#endif

	//二叉树算法分类
	while(1)
	{
		if(codeSize == CODE_BOOK_SIZE - 1)
			break;
		//分裂产生新码字
		for(i = 0; i < codeSize; i++)
		{
			if(codeSize * 2 == CODE_BOOK_SIZE && i == minDisIdx)
			{
				clusterMean[i] = (1.0 - SPLIT_PARAMETER) * clusterMean[i];
				//
				sumMean[i] = 0.0;
				sumNum[i] = 0;
				codeDis[i] = 0.0;
			}
			else
			{
				clusterMean[i+codeSize] = (1.0 + SPLIT_PARAMETER) * clusterMean[i];
				clusterMean[i] = (1.0 - SPLIT_PARAMETER) * clusterMean[i];
				//
				sumMean[i] = 0.0;
				sumNum[i] = 0;
				codeDis[i] = 0.0;
			}
		}
		
		if(codeSize * 2 == CODE_BOOK_SIZE)
			codeSize = codeSize * 2 - 1;
		else
			codeSize *= 2;
		//
		printf("码本大小: %d\n", codeSize);
		cycle = 0;
		//迭代，使当前大小的码本达到最优
		preSumDis = 0;
		relDis = 1.0;
		while(1)
		{
			curSumDis = 0;
			least3 = 0;
			//分类
			for(i = 0; i < sampleNum; i++)
			{			
				mindis = pos_huge;
				minidx = -1;
				for(k = 0; k < codeSize; k++)
				{
					dis = CalculateDistance(pSample[i], clusterMean[k]);
					if(dis < mindis)
					{
						mindis = dis;
						minidx = k;
					}
				}
				assert(minidx >= 0);
				curSumDis += mindis;
				codeDis[minidx] += mindis;
				sumNum[minidx]++;
				sumMean[minidx] += pSample[i];
			}
			//搜寻距离最大的类,如果该类的样本数比平均数小，则搜寻样本最多的类
			maxDis = min_huge;
			maxIdx = -1;
			for(i = 0; i < codeSize; i++)
			{
				codeDis[i] /= sumNum[i];
				if(codeDis[i] > maxDis)
				{
					maxDis = codeDis[i];
					maxIdx = i;
				}
			}
			if(sumNum[maxIdx] < 100)//sampleNum / CODE_BOOK_SIZE)
			{
				//搜寻样本最多的类
				maxCluster = 0;
				maxIdx = -1;
				for(i = 0; i < codeSize; i++)
				{
					if(sumNum[i] > maxCluster)
					{
						maxCluster = sumNum[i];
						maxIdx = i;
					}
				}
			}
			//搜寻类内距离最小的类,Appended on 8, May,2001.
			minDisIdx = -1;
			mindis = pos_huge;
			for(i = 0; i < codeSize; i++)
			{
				if(codeDis[i] < mindis)
				{
					mindis = codeDis[i];
					minDisIdx = i;
				}
			}

			//重估均值
			for(i = 0; i < codeSize; i++)
			{
				if(sumNum[i] < 4)
				{
					clusterMean[i] = (1.0 + SPLIT_PARAMETER) * sumMean[maxIdx] / (double)sumNum[maxIdx];
					least3++;
				}
				else
					clusterMean[i] = sumMean[i] / (double)sumNum[i];
			}
			memset(sumNum, 0, sizeof(int)*CODE_BOOK_SIZE);
			memset(sumMean, 0, sizeof(double)*CODE_BOOK_SIZE);
			memset(codeDis, 0, sizeof(double)*CODE_BOOK_SIZE);
			//
			ref = (curSumDis - preSumDis) / curSumDis;
			ref = fabs(ref);
			preSumDis = curSumDis;
			//
			cycle++;
			printf("迭代 %d, 距离: %f 误差 %f  样本数少于4: %d\n", cycle, curSumDis, ref, least3);
			if(ref < STOP_THRESHOLD)
				break;
		}
		//写文件
		/*
		maxDis = min_huge;
		for(i = 0; i < sampleNum; i++)
		{			
			mindis = pos_huge;
			minidx = -1;
			for(k = 0; k < codeSize; k++)
			{
				dis = CalculateDistance(pSample[i], clusterMean[k]);
				if(dis < mindis)
				{
					mindis = dis;
					minidx = k;
				}
			}
			if(mindis > maxDis)
				maxDis = mindis;
		}
		printf("最大误差: %f\n", maxDis * refVal);
		*/
		strcpy(name, "bookLm");
		_ltoa(codeSize, tmp, 10);
		strcat(name, tmp);
		strcat(name, ".dat");
		fp = fopen(name, "wb");
		if(fp == NULL)
		{
			printf("create error\n");
			exit(1);
		}
		for(i = 0; i < codeSize; i++)
		{
			prob = (long)(clusterMean[i] * refVal);
			fwrite(&prob, sizeof(long), 1, fp);
		}
		//写入特殊的值，主要是因为原来的数据中有此值，表示该值时语言模型概率不存在，可赋予极小值。
		//若语言模型中无特殊值，则下述代码可不写。
		//写入后，码本大小加一
		if(codeSize == CODE_BOOK_SIZE - 1)
		{
			prob = NEG_LM;
			fwrite(&prob, sizeof(long), 1, fp);
		}
		//
		fclose(fp);
	}
	
	delete	[]sumNum;
	delete	[]codeDis;
	delete []sumMean;
	delete []clusterMean;
}

//欧氏距离
double	CalculateDistance(double sample, double center)
{
	double dis, diffTemp, ftmp;

	diffTemp = sample - center;
	ftmp = diffTemp * diffTemp;
	dis = ftmp;
	return dis;
}

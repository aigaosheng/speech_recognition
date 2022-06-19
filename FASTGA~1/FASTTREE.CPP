/*********************************************************************************
说明：用于高斯的快速计算。HMM集合中的所有高斯作为分类样本，对高斯函数进行分类，
	然后对每个混合高斯函数构造一个short list表
**********************************************************************************/
#include	<stdio.h>
#include	<stdlib.h>
#include	<conio.h>
#include	<limits.h>
#include	<string.h>
#include	<math.h>
#include	<assert.h>

//#define		NO_OUTPUT	2788//2045//1632//2515//2733
#define		NO_DIM		42
#define		NO_MIX		16
//#define		TOTAL_GAUSSIAN	NO_OUTPUT*NO_MIX
#define		CODE_BOOK_SIZE	256//
#define		pos_huge	INT_MAX
#define		min_huge	INT_MIN
//#define		THRESHOLD	0.1 //似然度测度
#define		THRESHOLD	0.8//1.0	//欧氏距离测度
#define		MIN_SHORT_LIST	2//0//
//
#define		SPLIT_PARAMETER	0.02
#define		STOP_THRESHOLD	0.001//0.0001
//
float *pMean;			// Means[No_out][No_Mix][No_Dim]
float *pVariance;		// Variance[No_out][No_Mix][No_Dim];
float *pCoeMix;			// CoeMix[No_out][No_Mix];

int		LoadOutput(char*);
void	FastGaussianTree(void);
void	GenerateShortlist(void);
float	CalculateDistance(float *pMean, float* pVar, float *pCenter);
void	KmeansCluster(void);
void	BinaryCluster(void);

int NO_OUTPUT, TOTAL_GAUSSIAN;

void main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf("The programm needs 2 arguments:\n");
		printf("1. outputList.dat file\n");
		printf("2. the number of PDF\n");
		exit(-1);
	}
	NO_OUTPUT = atoi(argv[2]);
	TOTAL_GAUSSIAN = NO_OUTPUT*NO_MIX;

	pMean = new float[NO_OUTPUT*NO_MIX*NO_DIM];
	pVariance = new float[NO_OUTPUT*NO_MIX*NO_DIM];
	pCoeMix = new float[NO_OUTPUT*NO_MIX];

	if(!LoadOutput(argv[1]))
	{
		BinaryCluster();
		//ClusterGaussian();
		GenerateShortlist();
		//
		delete pMean;
		delete pVariance;
		delete pCoeMix;
	}
}

int LoadOutput(char *fileName)
{
	int	 i;

	FILE *OutputFile;

	//OutputFile=fopen("outputlist.dat", "rb");
	OutputFile=fopen(fileName, "rb");
	if(OutputFile==NULL)
	{
		printf("\nCan't open Outputlist.dat\n");
		return 1;
	}

	for(i=0;i<NO_OUTPUT;i++)
	{	
		fread(pCoeMix+i*NO_MIX, sizeof(float), NO_MIX, OutputFile);
		fread(pMean+i*NO_MIX*NO_DIM, sizeof(float), NO_MIX*NO_DIM, OutputFile);
		fread(pVariance+i*NO_MIX*NO_DIM,sizeof(float),NO_MIX*NO_DIM,OutputFile);
	}
	fclose(OutputFile);
	return 0;
}

void	BinaryCluster()
{
	int		i, k, l, sumNum[CODE_BOOK_SIZE], minidx, *vqIdx, cycle;
	double	*sumMean, *sumVar, dis, mindis, diffTemp, ftmp, maxDis,
			preSumDis, curSumDis, relDis, codeDis[CODE_BOOK_SIZE];
	float	clusterMean[CODE_BOOK_SIZE][NO_DIM], clusterVar[CODE_BOOK_SIZE][NO_DIM], ref;
	int least3, codeSize, maxIdx, maxCluster;

	sumMean = new double[CODE_BOOK_SIZE*NO_DIM];
	sumVar = new double[CODE_BOOK_SIZE*NO_DIM];
	vqIdx = new int[TOTAL_GAUSSIAN];
	//初始化
	memset(sumNum, 0, sizeof(int)*CODE_BOOK_SIZE);
	memset(sumMean, 0, sizeof(double)*NO_DIM*CODE_BOOK_SIZE);
	memset(sumVar, 0, sizeof(double)*NO_DIM*CODE_BOOK_SIZE);
	codeSize = 0;
	for(i = 0; i < TOTAL_GAUSSIAN; i++)
	{
		for(l = 0; l < NO_DIM; l++)
			sumMean[l] += pMean[i*NO_DIM+l];
	}
	for(l = 0; l < NO_DIM; l++)
		clusterMean[codeSize][l] = (float)(sumMean[l] / (float)(TOTAL_GAUSSIAN));
	//
	for(i = 0; i < TOTAL_GAUSSIAN; i++)
	{
		for(l = 0; l < NO_DIM; l++)
		{
			diffTemp = pMean[i*NO_DIM+l] - clusterMean[codeSize][l];
			sumVar[l] += diffTemp * diffTemp;
		}
	}
	for(l = 0; l < NO_DIM; l++)
		clusterVar[codeSize][l] = (float)(sumVar[l] / (float)(TOTAL_GAUSSIAN));
	codeSize++;
	//二叉树算法分类
	while(1)
	{
		if(codeSize == CODE_BOOK_SIZE)
			break;
		//分裂产生新码字
		for(i = 0; i < codeSize; i++)
		{
			for(l = 0; l < NO_DIM; l++)
			{
				clusterMean[i+codeSize][l] = (float)((1.0 + SPLIT_PARAMETER) * clusterMean[i][l]);
				clusterMean[i][l] = (float)((1.0 - SPLIT_PARAMETER) * clusterMean[i][l]);
				//
				sumMean[i*NO_DIM+l] = 0.0;
				sumVar[i*NO_DIM+l] = 0.0;
			}
			sumNum[i] = 0;
			codeDis[i] = 0.0;
		}
		codeSize *= 2;
		//
//		printf("码本大小: %d\n", codeSize);
		cycle = 0;
		//迭代，使当前大小的码本达到最优
		preSumDis = 0.0;
		relDis = 1.0;
		while(1)
		{
			curSumDis = 0.0;
			least3 = 0;
			//分类
			for(i = 0; i < TOTAL_GAUSSIAN; i++)
			{			
				mindis = pos_huge;
				minidx = -1;
				for(k = 0; k < codeSize; k++)
				{
					dis = CalculateDistance(pMean+i*NO_DIM, pVariance+i*NO_DIM, clusterMean[k]);
					if(dis < mindis)
					{
						mindis = dis;
						minidx = k;
					}
				}
				assert(minidx >= 0);
				curSumDis += mindis;
				codeDis[minidx] += mindis;
				vqIdx[i] = minidx;
				sumNum[minidx]++;
				for(l = 0; l < NO_DIM; l++)
					sumMean[minidx*NO_DIM+l] += pMean[i*NO_DIM+l];
			}
			//搜寻距离最大的类,如果该类的样本数比平均数小，则搜寻样本最多的类
			maxDis = min_huge;
			maxIdx = -1;
			for(i = 0; i < codeSize; i++)
			{
				codeDis[i] /= (float)(sumNum[i]);
				if(codeDis[i] > maxDis)
				{
					maxDis = codeDis[i];
					maxIdx = i;
				}
			}
			if(sumNum[maxIdx] < 100)//TOTAL_GAUSSIAN / CODE_BOOK_SIZE)
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
			//重估均值
			for(i = 0; i < codeSize; i++)
			{
				for(l = 0; l < NO_DIM; l++)
				{
					if(sumNum[i] < 4)
					{
						clusterMean[i][l] = (float)((1.0 + SPLIT_PARAMETER) * sumMean[maxIdx*NO_DIM+l] / (float)(sumNum[maxIdx]));
						least3++;
					}
					else
						clusterMean[i][l] = (float)(sumMean[i*NO_DIM+l] / (float)(sumNum[i]));
				}
			}
			//重估方差
			for(i = 0; i < TOTAL_GAUSSIAN; i++)
			{
				minidx = vqIdx[i];
				for(l = 0; l < NO_DIM; l++)
				{
					diffTemp = pMean[i*NO_DIM+l] - clusterMean[minidx][l];
					ftmp = diffTemp * diffTemp;
					sumVar[minidx*NO_DIM+l] += ftmp;
				}				
			}

			for(i = 0; i < codeSize; i++)
			{
				for(l = 0; l < NO_DIM; l++)
				{
					clusterVar[i][l] = (float)(sumVar[i*NO_DIM+l] / (float)(sumNum[i]));
					if(clusterVar[i][l] < 0.000001)
						clusterVar[i][l] = 0.000001;
					sumVar[i*NO_DIM+l] = 0.0;
					sumMean[i*NO_DIM+l] = 0.0;
				}
				sumNum[i] = 0;
				codeDis[i] = 0.0;
			}
			//
			ref = (float)((curSumDis - preSumDis) / curSumDis);
			ref = (float)fabs(ref);
			preSumDis = curSumDis;
			//
			cycle++;
			//printf("迭代 %d, 距离: %f 误差 %f  样本数少于4: %d\n", cycle, curSumDis, ref, least3);
			if(ref < STOP_THRESHOLD)
				break;
		}
	}
	FILE* fp = fopen("codeBook.dat", "wb");
	if(fp == NULL)
	{
		printf("create error\n");
		exit(1);
	}
	
//	fwrite(totalMean, sizeof(float), NO_DIM, fp);
//	fwrite(totalVar, sizeof(float), NO_DIM, fp);
	for(i = 0; i < CODE_BOOK_SIZE; i++)
	{
		fwrite(clusterMean[i], sizeof(float), NO_DIM, fp);
		fwrite(clusterVar[i], sizeof(float), NO_DIM, fp);
	}
	fclose(fp);
/*
	for(i = 0; i < CODE_BOOK_SIZE; i++)
	{
		for(j = 0; j < NO_DIM; j++)
			fprintf(fp, "%f ", clusterMean[i][j]);
		fprintf(fp, "\n");
		for(j = 0; j < NO_DIM; j++)
			fprintf(fp, "%f ", clusterVar[i][j]);
		fprintf(fp, "\n\n");		
	}
	fclose(fp);
	/*/

	delete []sumMean;
	delete []sumVar;
	delete []vqIdx;
}

void	KmeansCluster()
{
	int		i, j, k, l, KIte,*sumNum, *vecMix, sum, minidx, Diff;
	double	*sumMean, *sumVar, dis, mindis, diffTemp, ftmp;
	float	clusterMean[CODE_BOOK_SIZE][NO_DIM], clusterVar[CODE_BOOK_SIZE][NO_DIM], totalMean[NO_DIM], totalVar[NO_DIM];
	int least3;

	sumNum = new int[CODE_BOOK_SIZE];
	sumMean = new double[CODE_BOOK_SIZE*NO_DIM];
	sumVar = new double[CODE_BOOK_SIZE*NO_DIM];
	vecMix = new int[TOTAL_GAUSSIAN];

	//计算所有高斯的均值和方差
	memset(totalMean, 0, sizeof(float)*NO_DIM);
	memset(totalVar, 0, sizeof(float)*NO_DIM);
	for(i = 0; i < TOTAL_GAUSSIAN; i++)
	{
		for(l = 0; l < NO_DIM; l++)
			totalMean[l] += pMean[i*NO_DIM+l];
	}
	for(l = 0; l < NO_DIM; l++)
		totalMean[l] /= TOTAL_GAUSSIAN;

	for(i = 0; i < TOTAL_GAUSSIAN; i++)
	{
		for(l = 0; l < NO_DIM; l++)
		{
			diffTemp = pMean[i*NO_DIM+l] - totalMean[l];
			totalVar[l] += (float)(diffTemp * diffTemp);
		}
	}
	for(l = 0; l < NO_DIM; l++)
		totalVar[l] /= TOTAL_GAUSSIAN;

	//K均值初始化
	for(i = 0; i < TOTAL_GAUSSIAN; i++)
		vecMix[i] = -1;

	sum = 0;
	for(i = 0; i < TOTAL_GAUSSIAN; i++)
	{
		vecMix[i] = sum  % CODE_BOOK_SIZE;
		sum++;
	}
	//K均值迭代
	for(KIte = 0; KIte < 10; KIte++)
	{
		printf("第 %d 次迭代 ", KIte);
		least3 = 0;

		Diff = 0;
		for(j = 0; j < CODE_BOOK_SIZE; j++)
		{
			sumNum[j] = 0; // vector number in the jth cluster
			for(l = 0; l < NO_DIM; l++)
				sumMean[j*NO_DIM+l] = 0; // mean value in the jth cluster
		}

		//计算样本的和
		for(i = 0; i < TOTAL_GAUSSIAN; i++)
		{
			sumNum[vecMix[i]]++;
			for(l = 0; l < NO_DIM; l++)
				sumMean[vecMix[i]*NO_DIM+l] += pMean[i*NO_DIM+l];
		}

		//查找样本最多的类
		int MaxNum,MaxIdx,MaxI;
		MaxNum = 0;
		MaxIdx = 0;
		for(k = 0; k < CODE_BOOK_SIZE;k++)
		{
			if(sumNum[k] > MaxNum)
			{
				MaxNum = sumNum[k];
				MaxIdx = k;
			}
			MaxI = MaxIdx;
		}
		//计算每一类的均值
		for(k = 0; k < CODE_BOOK_SIZE; k++)
		{
			for(l = 0; l < NO_DIM; l++)
			{
				if(sumNum[k] <= 3) // if the cluster is too small
				{
					ftmp = 0;
					for(int zz = 0; zz < CODE_BOOK_SIZE; zz++)
						ftmp += sumMean[zz*NO_DIM+l];
					clusterMean[k][l] = (float)(ftmp / sum);
					//
					least3++;
				}
				else
					clusterMean[k][l] = (float)(sumMean[k*NO_DIM+l] / sumNum[k]);
			}
		}
		//计算方差
		for(k = 0; k < CODE_BOOK_SIZE; k++)
			for(l = 0;l < NO_DIM; l++)
				sumVar[k*NO_DIM+l] = 0;

		for(i = 0; i < TOTAL_GAUSSIAN; i++)
		{
			for(l = 0;l < NO_DIM; l++)
			{
				diffTemp = pMean[i*NO_DIM+l] - clusterMean[vecMix[i]][l];
				sumVar[vecMix[i]*NO_DIM+l] += diffTemp * diffTemp;
			}
		}
		for(k = 0; k < CODE_BOOK_SIZE; k++)
		{
			for(l = 0;l < NO_DIM; l++)
			{
				if(sumNum[k] <= 3)
				{
					ftmp = 0;
					for(int zz = 0; zz < CODE_BOOK_SIZE; zz++)
						ftmp += sumVar[zz*NO_DIM+l];
					clusterVar[k][l] = (float)(ftmp / sum);
					if(clusterVar[k][l] < 1.0e-3)
						clusterVar[k][l] = (float)1.0e-3;
				}
				else
				{
					clusterVar[k][l] = (float)(sumVar[k*NO_DIM+l] / sumNum[k]);
					if(clusterVar[k][l] < 1.0e-3)
						clusterVar[k][l] = (float)1.0e-3;
				}
			}
		}

		//根据新计算出的形心重新分类
		for(i = 0; i < TOTAL_GAUSSIAN; i++)
		{
			mindis = pos_huge;
			for(k = 0; k < CODE_BOOK_SIZE; k++)
			{
				dis = CalculateDistance(pMean+i*NO_DIM, pVariance+i*NO_DIM, clusterMean[k]);
				if(dis < mindis)
				{
					mindis = dis;
					minidx = k;
				}
			}
	
			if(minidx != vecMix[i])
				Diff++; // record the number of vectors which are clustered into different cluster 
						// from the previous.

			vecMix[i] = minidx;
		} 
		//
		printf("样本数小于3的类数 %d\n", least3);
		//
		if(Diff <= CODE_BOOK_SIZE)
			break;
	}
	FILE* fp = fopen("codeBook.dat", "wb");
	if(fp == NULL)
	{
		printf("create error\n");
		exit(1);
	}
	
	//fwrite(totalMean, sizeof(float), NO_DIM, fp);
	//fwrite(totalVar, sizeof(float), NO_DIM, fp);
	for(i = 0; i < CODE_BOOK_SIZE; i++)
	{
		fwrite(clusterMean[i], sizeof(float), NO_DIM, fp);
		fwrite(clusterVar[i], sizeof(float), NO_DIM, fp);
	}
	fclose(fp);
/*
	for(i = 0; i < CODE_BOOK_SIZE; i++)
	{
		for(j = 0; j < NO_DIM; j++)
			fprintf(fp, "%f ", clusterMean[i][j]);
		fprintf(fp, "\n");
		for(j = 0; j < NO_DIM; j++)
			fprintf(fp, "%f ", clusterVar[i][j]);
		fprintf(fp, "\n\n");		
	}
	fclose(fp);
	/*/

	delete []sumNum;
	delete []sumMean;
	delete []sumVar;
	delete []vecMix;
}

void	GenerateShortlist()
{
	int		i, j, k, l, countSum;
	float	clusterMean[CODE_BOOK_SIZE][NO_DIM], clusterVar[CODE_BOOK_SIZE][NO_DIM], totalMean[NO_DIM], totalVar[NO_DIM],
			disVal[NO_MIX], minDis;
	short	*count, sort[NO_MIX], tmpInt, minIdx;
	//char	index[NO_OUTPUT][NO_MIX];
	char	*index;
	int		offset;
	FILE	*fp, *fidx;

	count = new short[NO_OUTPUT];
	index = new char[NO_OUTPUT*NO_MIX];
	//disVal = new float[NO_OUTPUT*NO_MIX];

//	memset(totalMean, 0, sizeof(float)*NO_DIM);
//	memset(totalVar, 0, sizeof(float)*NO_DIM);
	
	fp = fopen("codeBook.dat", "rb");
	if(fp == NULL)
	{
		printf("create error\n");
		return;
	}
//	fread(totalMean, sizeof(float), NO_DIM, fp);
//	fread(totalVar, sizeof(float), NO_DIM, fp);
	for(i = 0; i < CODE_BOOK_SIZE; i++)
	{
		fread(clusterMean[i], sizeof(float), NO_DIM, fp);
		fread(clusterVar[i], sizeof(float), NO_DIM, fp);
	}
	fclose(fp);
	/*/
	FILE* fout = fopen("debug.txt", "wt");
	if(fout == NULL)
	{
		printf("debug.txt open error\n");
		return;
	}
	for(i = 0; i < NO_OUTPUT; i++)
	{
		for(j = 0; j < NO_MIX; j++)
		{
			minDis = INT_MAX;
			minIdx = 0;
			for(k = 0; k < CODE_BOOK_SIZE; k++)
			{
				disVal[0] = CalculateDistance(pMean+i*NO_MIX*NO_DIM+j*NO_DIM, pVariance+i*NO_MIX*NO_DIM+j*NO_DIM, clusterMean[k]);
				if(disVal[0] < minDis)
				{
					minDis = disVal[0];
					minIdx = k;
				}
			}
			fprintf(fout, "%-5d%-3d:%-5d %f\n", i, j, minIdx, minDis);
		}
	}
	fclose(fout);
*/
	//计算每个输出分布的short list
	fp = fopen("shortList.dat", "wb");
	fidx = fopen("shortList.idx", "wb");
	if(fp == NULL || fidx == NULL)
	{
		printf("create error\n");
		return;
	}
	int totalSum = 0;
	offset = 0;
	for(k = 0; k < CODE_BOOK_SIZE; k++)
	{
		//memset(index, -1, sizeof(char)*NO_OUTPUT);
		memset(index, -1, sizeof(char)*NO_OUTPUT*NO_MIX);
		memset(count, 0, sizeof(short)*NO_OUTPUT);
		countSum = 0;
		for(i = 0; i < NO_OUTPUT; i++)
		{
			for(j = 0; j < NO_MIX; j++)
				disVal[j] = CalculateDistance(pMean+i*NO_MIX*NO_DIM+j*NO_DIM, pVariance+i*NO_MIX*NO_DIM+j*NO_DIM, clusterMean[k]);
			//根据距离大小排序
			for(j = 0; j < NO_MIX; j++)
				sort[j] = j;
			for(j = 0; j < NO_MIX - 1; j++)
			{
				minDis = disVal[j];
				minIdx = j;
				for(l = j + 1; l < NO_MIX; l++)
				{
					if(minDis > disVal[l])
					{
						minIdx = l;
						minDis = disVal[l];
					}
				}
				disVal[minIdx] = disVal[j];
				disVal[j] = minDis;
				tmpInt = sort[j];
				sort[j] = sort[minIdx];
				sort[minIdx] = tmpInt;
			}
			/*
			if(disVal[0] >= THRESHOLD)
			{
				for(j = 0; j < 2; j++)
				{
					index[i][count[i]] = sort[j];
					count[i]++;
					countSum++;
				}
				
			}
			else
			{
				for(j = 0; j < NO_MIX; j++)
				//for(j = 0; j < 8; j++)
				{
					if(disVal[j] >= THRESHOLD)
						continue;
					index[i][count[i]] = sort[j];
					count[i]++;
					countSum++;
				}
				if(count[i] <= 1)
				{
					index[i][count[i]] = sort[1];
					count[i]++;
				}
			}
			*/
			//if(k>180&&k<250)fprintf(fout, "%-5d%-5d ", k, i);
			for(j = 0; j < MIN_SHORT_LIST; j++)
			{
			//	index[i][count[i]] = sort[j];
				index[i*NO_MIX+count[i]] = sort[j];
				count[i]++;
				countSum++;
				//
				//if(k>180&&k<250)fprintf(fout, "%f ", disVal[j]);
			}

			for(j = MIN_SHORT_LIST; j < NO_MIX; j++)
		//	for(j = MIN_SHORT_LIST; j < 8; j++)
			{
				//if(k>180&&k<250)fprintf(fout, "%f ", disVal[j]);
				//
				if(disVal[j] >= THRESHOLD)
					continue;
				index[i*NO_MIX+count[i]] = sort[j];
				count[i]++;
				countSum++;
			}
			//if(k>180&&k<250)fprintf(fout, "\n");
		}
		//printf("%d\n", countSum);


		totalSum += countSum;
		for(i = 0; i < NO_OUTPUT; i++)
		{
			fwrite(count+i, sizeof(short), 1, fidx);
			fwrite(&offset, sizeof(int), 1, fidx);
			//fwrite(index+i, sizeof(char), count[i], fp);
			fwrite(index+i*NO_MIX, sizeof(char), count[i], fp);
			offset += count[i];
		}
		//if(k>=250)fclose(fout);
	}
//	fwrite(&totalSum, sizeof(int), 1, fp);
	fclose(fp);
	fclose(fidx);
	totalSum /= (NO_OUTPUT*CODE_BOOK_SIZE);
	//printf("%d\n", totalSum);

	delete count;
	delete index;
}

/*/距离测度
float	CalculateDistanceT(float *pmean, float *pVar, float *pCenter)
{
	float dis;
	double	diffTemp, ftmp;
	int	l;

	dis = 0;
	for(l = 0; l < NO_DIM; l++)  // Distance Calculation Option
	{
		diffTemp = pmean[l] - pCenter[l];
		ftmp = diffTemp * diffTemp;
		//dis += ftmp / sqrt(totalVar[l]*clusterVar[k][l]);
		//dis += ftmp;
		dis += (float)(ftmp / pVar[l]);
	}
	dis = (float)(dis / (float)(NO_DIM));
	return dis;
}
*/
//似然度距离测度
float	CalculateDistanceLike(float *pmean, float *pVar, float *pCenter)
{
	float dis;
	double	diffTemp, ftmp;
	int	l;

	dis = 0;
	for(l = 0; l < NO_DIM; l++)  // Distance Calculation Option
	{
		diffTemp = pmean[l] - pCenter[l];
		ftmp = diffTemp * diffTemp;
		dis += (float)(ftmp / pVar[l] + log(pVar[l]));
//		dis += (float)log(pVar[l]);
	}
	dis = dis / (float)(NO_DIM);
	return dis;
}

//欧氏距离
float	CalculateDistance(float *pmean, float *pVar, float *pCenter)
{
	float dis;
	double	diffTemp, ftmp;
	int	l;

	dis = 0;
	for(l = 0; l < NO_DIM; l++)  // Distance Calculation Option
	{
		diffTemp = pmean[l] - pCenter[l];
		ftmp = diffTemp * diffTemp;
		dis += (float)ftmp;
	}
	dis = dis / (float)(NO_DIM);
	return dis;
}

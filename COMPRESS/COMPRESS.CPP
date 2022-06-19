/////////////////////////////////////////////////////////////
// Program Name: compress.cpp
// Description: compress data
// date: July 24,2000
////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include "const.h"

int CompressTriphoneMap(void);
int CompressTriphoneMapTwoFile(void);

int triphoneMap[TOTAL_PHONES][5][6][6][TOTAL_PHONES][TOTAL_PHONES];

void main()
{
	CompressTriphoneMap();
	//CompressTriphoneMapTwoFile();

}

//写入一个文件
int CompressTriphoneMap(void)
{
	int len;
	int i, l, r, it, lt, rt;
	FILE *f1;

	f1 = fopen("triphoneList.dat", "rb");
	//f1 = fopen("data\\triphoneList.dat", "rb");
	if(f1 == NULL)
	{
		printf("triphoneList.dat not found\n");
		getch();
		exit(-1);
	}
	for(i = 0; i <= MAX_CODE_INITIAL_PART; i++)
		fread(triphoneMap[i][0][0][0], sizeof(int), TOTAL_PHONES*TOTAL_PHONES, f1);	
	fread(triphoneMap+MAX_CODE_INITIAL_PART+1, sizeof(int), 5*6*6*TOTAL_PHONES*TOTAL_PHONES*(MAX_CODE_FINAL_PART-MAX_CODE_INITIAL_PART), f1);	
	fread(triphoneMap[SILENCE_CODE][0][0][0], sizeof(int), TOTAL_PHONES*TOTAL_PHONES, f1);
	fclose(f1);
	//
	int ct=0;
	f1 = fopen("triphoneMap.dat", "wb");
	if(f1 == NULL)
	{
		printf("triphoneList.dat not found\n");
		getch();
		exit(-1);
	}
	for(i = 0; i <= MAX_CODE_INITIAL_PART; i++)
	{
		for(l = MAX_CODE_INITIAL_PART+1; l < TOTAL_PHONES; l++)
		{
			for(r = MAX_CODE_INITIAL_PART+1; r <= MAX_CODE_FINAL_PART; r++)
			{
			//	if(triphoneMap[i][0][0][0][l][r] > -1)
			//	{
					fwrite(&triphoneMap[i][0][0][0][l][r], sizeof(short), 1, f1);
					ct++;
			//	}
			}
		}
	}
	printf("%d\n", ct);
	for(i = MAX_CODE_INITIAL_PART + 1; i <= MAX_CODE_FINAL_PART; i++)
	{
		for(it=0;it<5;it++)
			for(lt=0;lt<6;lt++)
				for(rt=0;rt<6;rt++) 
				{
					for(l = 0; l <= MAX_CODE_INITIAL_PART; l++)
					{
						for(r = 0; r < TOTAL_PHONES; r++)
						{
							if(r > MAX_CODE_INITIAL_PART && r <= MAX_CODE_FINAL_PART)
								continue;
						//	if(triphoneMap[i][it][lt][rt][l][r] > -1)
						//	{
								fwrite(&triphoneMap[i][it][lt][rt][l][r], sizeof(short), 1, f1);
								ct++;
						//	}
						}
					}
				}
	}
	printf("%d\n", ct);
	/*for(l = MAX_CODE_INITIAL_PART+1; l <= MAX_CODE_FINAL_PART; l++)
	{
		for(r = 0; r <= MAX_CODE_INITIAL_PART; r++)
		{
			//if(triphoneMap[61][0][0][0][l][r] > -1)
			//{
				fwrite(&triphoneMap[61][0][0][0][l][r], sizeof(short), 1, f1);
				ct++;
			//}
		}
	}
*/
	fclose(f1);
	printf("%d\n", ct);
	//getch();
	return 0;
}

int CompressTriphoneMapTwoFile(void)
{
	int len;
	int i, l, r, it, lt, rt;
	FILE *f1;

	//f1 = fopen("d:\\gsh\\decoder\\data\\triphoneList.dat", "rb");
	f1 = fopen("triphoneList.dat", "rb");
	if(f1 == NULL)
	{
		printf("triphoneList.dat not found\n");
		getch();
		exit(-1);
	}
	for(i = 0; i <= MAX_CODE_INITIAL_PART; i++)
		fread(triphoneMap[i][0][0][0], sizeof(int), TOTAL_PHONES*TOTAL_PHONES, f1);	
	fread(triphoneMap+MAX_CODE_INITIAL_PART+1, sizeof(int), 5*6*6*TOTAL_PHONES*TOTAL_PHONES*(MAX_CODE_FINAL_PART-MAX_CODE_INITIAL_PART), f1);	
	fread(triphoneMap[SILENCE_CODE][0][0][0], sizeof(int), TOTAL_PHONES*TOTAL_PHONES, f1);
	fclose(f1);
	//
	int ct=0;
	//f1 = fopen("d:\\gsh\\decoder\\data\\triphoneMapI.dat", "wb");
	f1 = fopen("triphoneMapI.dat", "wb");
	if(f1 == NULL)
	{
		printf("triphoneList.dat not found\n");
		getch();
		exit(-1);
	}
	for(i = 0; i <= MAX_CODE_INITIAL_PART; i++)
	{
		for(l = MAX_CODE_INITIAL_PART+1; l < TOTAL_PHONES; l++)
		{
			for(r = MAX_CODE_INITIAL_PART+1; r <= MAX_CODE_FINAL_PART; r++)
			{
			//	if(triphoneMap[i][0][0][0][l][r] > -1)
			//	{
					fwrite(&triphoneMap[i][0][0][0][l][r], sizeof(unsigned short), 1, f1);
					ct++;
			//	}
			}
		}
	}
	fclose(f1);
	printf("%d\n", ct);
	//f1 = fopen("d:\\gsh\\decoder\\data\\triphoneMapF.dat", "wb");
	f1 = fopen("triphoneMapF.dat", "wb");
	if(f1 == NULL)
	{
		printf("triphoneList.dat not found\n");
		getch();
		exit(-1);
	}
	for(i = MAX_CODE_INITIAL_PART + 1; i <= MAX_CODE_FINAL_PART; i++)
	{
		for(it=0;it<5;it++)
			for(lt=0;lt<6;lt++)
				for(rt=0;rt<6;rt++) 
				{
					for(l = 0; l <= MAX_CODE_INITIAL_PART; l++)
					{
						for(r = 0; r < TOTAL_PHONES; r++)
						{
							if(r > MAX_CODE_INITIAL_PART && r <= MAX_CODE_FINAL_PART)
								continue;
						//	if(triphoneMap[i][it][lt][rt][l][r] > -1)
						//	{
								fwrite(&triphoneMap[i][it][lt][rt][l][r], sizeof(unsigned short), 1, f1);
								ct++;
						//	}
						}
					}
				}
	}
	printf("%d\n", ct);
	/*for(l = MAX_CODE_INITIAL_PART+1; l <= MAX_CODE_FINAL_PART; l++)
	{
		for(r = 0; r <= MAX_CODE_INITIAL_PART; r++)
		{
			//if(triphoneMap[61][0][0][0][l][r] > -1)
			//{
				fwrite(&triphoneMap[61][0][0][0][l][r], sizeof(short), 1, f1);
				ct++;
			//}
		}
	}
*/
	fclose(f1);
	printf("%d\n", ct);
	getch();
	return 0;
}

/*******************************************************************************
Version 1.0
File Name: lextree.cpp
Description: this program is to generate lexical tree from dictionary.
Written by: Gao sheng
Date: Nov.2, 1999
Date Modified: June 29,2000
Note: because pitch information is used.  FINAL and INITIAL are all divided into 5 tonal
FINALs and INITIALs. Each node in the tree represents a tonal base phone(INITIAL or FINAL)
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "include\struct.h"
#include "include\global.h"
#include "include\const.h"

//#define STORE_MAX_PROB

//#define	DICTIONARY_SIZE	39969//31//
//#define BEGIN_END_CODE	TOTAL_PITCH_PHONES-1
int dictionarySize, functionArgu, beginEndCode, totalPitchPhones;

extern int		FindSyllableIndex(char*);
extern int		LoadSyllableTable(char*);
extern int		FindSyllableIndex(char*);
extern int		FindPhoneIndex(char*);
extern int		LoadPhones(char*);

int FindPitchPhone306(int phone, char pitch, int* pitchPhStack, char* pitchStack);
int FindPitchPhone210(int phone, char pitch, int* pitchPhStack, char* pitchStack);

void SortDictionary(void);
void ConvertToModelString(int);
int ConvertToPhoneModelString(char** pinYinString, int len, int *phoneIdx, char*, int &phoneNum);
void ChangeUnigramToXU(void);
void GenerateLexicalTree(void);
void UpdateUnigram(void);

CLexicalTree  tree[MAX_TREE_NODE];

//
void GenerateBasephoneSet210(void);
void GenerateBasephoneSet306(void);
void GenerateBasephoneSet394(void);

//
void main(int argv, char *argc[])
{
	if(argv != 3)
	{
		printf("Need two arguments: function argument, 词典中词的数目\n");
		printf("These are function arguments:\n");
		printf("  0：产生有调210个Base phone（声母无调24个、韵母有调37*5个，1个Silence\n");
		printf("  1：用210个base phone转换词典\n");
		printf("  2：用210个base phone生成词树\n");
		printf("  3：产生306个有调Base phone（声母有调24*5个、韵母有调37*5个，1个Silence\n");
		printf("  4：用306个base phone转换词典\n");
		printf("  5：用306个base phone生成词树\n");
		return;
	}
	functionArgu = atoi(argc[1]);
	dictionarySize = atoi(argc[2]);

	switch(functionArgu)
	{
		case 0:
			GenerateBasephoneSet210();
			break;
		case 1:
			ConvertToModelString(1);
			SortDictionary();
			break;
		case 2:
			UpdateUnigram();
			GenerateLexicalTree();
			break;
		case 3:
			GenerateBasephoneSet306();
			break;
		case 4:
			ConvertToModelString(3);
			SortDictionary();
			break;
		case 5:
			UpdateUnigram();
			GenerateLexicalTree();
			break;
		default:
			break;
	}
}

void GenerateBasephoneSet210()
{
	//in the follwoing schedue only tonaless FINAL is divided into 5 tonal FINALs and INITIAL
	//is toneless.
	int map[210];
	char pitchmap[210];
	for(int ii = 0; ii < TOTAL_INITIAL; ii++)
	{
		map[ii] = ii;
		pitchmap[ii] = -1;
	}
	for(ii = TOTAL_INITIAL; ii < SILENCE_CODE;ii++)
		for(int jj = 0; jj < 5; jj++)
	{
		map[(ii-TOTAL_INITIAL)*5+jj+TOTAL_INITIAL] = ii;
		pitchmap[(ii-TOTAL_INITIAL)*5+jj+TOTAL_INITIAL] = jj;
	}

	map[209] = SILENCE_CODE;
	pitchmap[209] = -1;
	FILE *fh = fopen("pitchPhoneMap.dat","wb");
	if(fh == NULL)
	{
		printf("error when creating pitchPhoneMap.dat\n");
		getch();
		return;
	}
	fwrite(map, sizeof(int), 210, fh);
	fwrite(pitchmap, sizeof(char), 210,fh);
	fclose(fh);
}

void GenerateBasephoneSet394()
{
	//词首和词尾的韵母分别标记不同的码,而原来是不区分词首和词尾的韵母,June 19,2000
	int map[394], ii, jj;
	char pitchmap[394];
	for(ii = 0; ii < TOTAL_INITIAL; ii++)
	{
		map[ii] = ii;
		pitchmap[ii] = -1;
	}
	for(ii = TOTAL_INITIAL; ii < SILENCE_CODE; ii++)
		for(jj = 0; jj < 5; jj++)
		{
			map[(ii-TOTAL_INITIAL)*5+jj+TOTAL_INITIAL] = ii;
			pitchmap[(ii-TOTAL_INITIAL)*5+jj+TOTAL_INITIAL] = jj;
		}

	for(ii = TOTAL_INITIAL; ii < SILENCE_CODE; ii++)
		for(jj = 0; jj < 5; jj++)
		{
			map[(ii-TOTAL_INITIAL)*5+jj+209] = ii;
			pitchmap[(ii-TOTAL_INITIAL)*5+jj+209] = jj + 5;
		}
	FILE *fh=fopen("pitchPhoneMap.dat","wb");
	if(fh == NULL)
	{
		printf("error when creating pitchPhoneMap.dat\n");
		getch();
		return;
	}
	fwrite(map, sizeof(int), 394, fh);
	fwrite(pitchmap, sizeof(char), 394, fh);
	fclose(fh);
}

void GenerateBasephoneSet306()
{
	//in the follwoing schedue INITIAL and FINAL are divided into 5 tonal FINALs
	int map[306],ii,jj;
	char pitchmap[306];

	for(ii = 0; ii < SILENCE_CODE; ii++)
		for(jj = 0; jj < 5; jj++)
		{
			map[ii*5+jj] = ii;
			pitchmap[ii*5+jj] = jj;
		}

	map[305] = SILENCE_CODE;
	pitchmap[305] = -1;
	FILE *fh=fopen("pitchPhoneMap.dat","wb");
	if(fh == NULL)
	{
		printf("error when creating pitchPhoneMap.dat\n");
		getch();
		return;
	}
	fwrite(map, sizeof(int), 306, fh);
	fwrite(pitchmap, sizeof(char), 306, fh);
	fclose(fh);
}

//Description: this function is to generate format dictionary for making lexical tree and convert 
// pronunciation of words into syllable code string
//format: hanzi, xrange mark+syllable code string corresponding to word, unigram, weight when backuping
//its bigram to unigram

//Description: the function is to sort dictionay according to decreasing syllable code
//
void SortDictionary()
{
	FILE *f1, *f2;
	long i, j, jj, num, sameCt;
	int *oldToNew = new int[dictionarySize];

	typedef struct{
		char hanzi[21];
		char pitch[50];
		int len;
		int code[50];
		long  unigram;
		int oldWordCode;
	}CDictionary;

	CDictionary *dic = new CDictionary[dictionarySize], temp;
	int str1[11], str2[11];

	f1 = fopen("lexical.txt", "rt");
	f2 = fopen("pitchDic.txt", "rt");
	if(f1 == NULL || f2 == NULL)
	{
		printf("open dic error\n");
		return;
	}
	for(i = 0; i < dictionarySize; i++)
	{
		fscanf(f1, "%s%d", dic[i].hanzi, &dic[i].len);
		for(j = 0; j < dic[i].len; j++)
		{
			fscanf(f1, "%d", &dic[i].code[j]);
			fscanf(f2, "%d", &dic[i].pitch[j]);
		}
		fscanf(f1, "%d", &dic[i].unigram);
		dic[i].oldWordCode = i;
	}
	fclose(f1);
	fclose(f2);

	for(i = 0; i < dictionarySize-1; i++)
	{
		printf("%d\n", i);

		memcpy(str1, dic[i].code, sizeof(int)*dic[i].len);
		for(j = i+1; j < dictionarySize; j++)
		{
			memcpy(str2, dic[j].code, sizeof(int)*dic[j].len);
			num = __min(dic[j].len, dic[j].len);
			sameCt = 0;
			for(jj = 0; jj < num; jj++)
			{
				if(dic[i].code[jj] > dic[j].code[jj])
				{
					temp = dic[j];
					dic[j] = dic[i];
					dic[i] = temp;
					break;
				}
				else if(dic[i].code[jj] == dic[j].code[jj])
				{
					sameCt++;
					continue;
				}
				else 
					break;
			}
			if(sameCt == num)
			{
				if(dic[i].len > dic[j].len)
				{
					temp = dic[j];
					dic[j] = dic[i];
					dic[i] = temp;
				}
			}
		}
	}

		f1 = fopen("lexicalSorted.txt", "wt");
		f2 = fopen("cihaomap.dat", "wb");
		FILE *f4 = fopen("pitchDicSort.txt", "wt");
		FILE *f3 = fopen("hanzi.han", "wt");
		if(f1 == NULL || f2 == NULL || f3 == NULL || f4 == NULL)
		{
			printf("create dic error\n");
			return;
		}
		for(i = 0; i < dictionarySize; i++)
		{
			fprintf(f1, "  %-16s%-6d", dic[i].hanzi, dic[i].len);
			for(j = 0; j < dic[i].len; j++)
			{
				fprintf(f1, "%-6d", dic[i].code[j]);
				fprintf(f4, "%-6d", dic[i].pitch[j]);
			}
			fprintf(f4, "\n");
			fprintf(f1, " %d %d\n", dic[i].unigram, dic[i].oldWordCode);
			fprintf(f3, "%s\n", dic[i].hanzi);
			//fprintf(f2, "%d  %d\n", i, dic[i].oldWordCode);
			//fprintf(f1, " 999  %d %d\n", dic[i].unigram, dic[i].oldWordCode);
			oldToNew[dic[i].oldWordCode] = i;
		}
		for(i = 0; i < dictionarySize; i++)
		{
			fwrite(&dic[i].oldWordCode, sizeof(int), 1, f2);
		}
		fwrite(oldToNew, sizeof(int), dictionarySize, f2);
		
		fclose(f1);
		fclose(f2);
		fclose(f3);
		fclose(f4);
		delete dic;
		delete oldToNew;
}

//Description: the function is to generate lexical tree from the sorted dictionary
//root node: Silence Model 
//每个叶节点都是一个triphone model的节点，其startNode=-1, linkNum:同音词的数目,wordFirst和wordLast同其他节点意义一样
void GenerateLexicalTree()
{
	FILE *f1, *f2;
	int i, j, nodeCt, leftCt, preNode, ii;
	int nodeCtTmp;
		
	int totalNode;
	int xrange;
	int	yrange;
	int linkNumTmp;

	typedef struct{
		char hanzi[21];
		char pitch[50];
		int len;
		int code[50];
		long  unigram;
		int oldWordCode;
	}CSortedDictionary;

	CSortedDictionary *dic = new CSortedDictionary[dictionarySize];
	
	f1 = fopen("lexicalSortedUpdate.txt", "rt");
	f2 = fopen("pitchDicSort.txt", "rt");
	if(f1 == NULL || f2 == NULL)
	{
		printf("open dic error\n");
		return;
	}
	for(i = 0; i < dictionarySize; i++)
	{
		fscanf(f1, "%s%d", dic[i].hanzi, &dic[i].len);
		//dic[i].len = strlen(dic[i].hanzi) / 2 + 2;
		for(j = 0; j < dic[i].len; j++)
		{
			fscanf(f1, "%d", &dic[i].code[j]);
			fscanf(f2, "%d", &dic[i].pitch[j]);
		}
		fscanf(f1, "%d%d", &dic[i].unigram, &dic[i].oldWordCode);
	}
	fclose(f1);
	fclose(f2);
	//
	FILE* fpt = fopen("pitchPhoneMap.dat", "rb");
	if(fpt == NULL)
	{
		printf("pitch phone map file opened error\n");
		exit(-1);
	}
	fseek(fpt, 0, SEEK_END);
	totalPitchPhones = ftell(fpt) / (sizeof(int) + sizeof(char));
	int *pitchPhoneMap = new int[totalPitchPhones];
	char *pitchMap = new char[totalPitchPhones];
	fseek(fpt, 0, SEEK_SET);
	fread(pitchPhoneMap, sizeof(int), totalPitchPhones, fpt);
	fread(pitchMap, sizeof(char), totalPitchPhones, fpt);
	fclose(fpt);
	//
	beginEndCode = totalPitchPhones - 1;
	//
	nodeCt = 0;
	tree[0].nodeId = beginEndCode;
	tree[0].toneId = PITCH_NULL;
	nodeCt++;

	int *isLeaf = new int[dictionarySize];
	int leftWord;
	int *father = new int[dictionarySize];
	long maxprob, maxidx;

	for(i = 0; i < dictionarySize; i++)
	{
		isLeaf[i] = 0; //if=1, the word reaches its yrange.
		father[i] = 0;
	}
	leftCt = 0;
	int layerCt = 1;
	int prefather = -1;
	while(1)
	{
		//检查所有的词是否已经处理完。如处理完，就退出while()
		leftCt = 0;
		for(i = 0 ; i < dictionarySize; i++)
		{
			if(isLeaf[i])
				continue;
			leftCt++;
		}
		if(!leftCt)
			break;

		//Update father's information.计算当前层的节点数，并与其父节点联系上，填写父节点的有关参数
		//找出第一个没有处理完的词，并记录其父节点
		for(ii = 0 ; ii < dictionarySize; ii++)
		{
			if(isLeaf[ii])
				continue;
			prefather = father[ii];
			xrange = ii;
			break;
		}

		totalNode = nodeCt;
		yrange = xrange;
		linkNumTmp = 0;
		for(i =  xrange; i < dictionarySize; i++)
		{
			if(isLeaf[i])
				continue;
			if(father[i] == prefather) //all words sharing this father
			{
				linkNumTmp++; //word number.记录父节点可到达的词个数
				yrange = i; //记录父节点可到达的最后一个词
			}
			else //different father node occur。新的父节点开始，保存已经查询过的父节点的信息
			{
				//save information of checked father node
				if(linkNumTmp != yrange-xrange+1)
				{
					getch();
					return;
				}
				if(tree[prefather].nodeId == beginEndCode && layerCt > 1)
					tree[prefather].startNode = -1;
				else
					tree[prefather].startNode = nodeCt; //the xrange node of its father

				nodeCtTmp = nodeCt;
				//搜索该父节点连接的所有子节点的树目，以获得该父节点的第一个子节点的节点编码
				maxprob = neg_huge;
				maxidx = -1;
				preNode = -1;
				for(ii = xrange; ii <= yrange; ii++)
				{
					if(dic[ii].code[layerCt] != preNode)
					{
						preNode = dic[ii].code[layerCt];
						nodeCt++;
						assert(nodeCt<MAX_TREE_NODE);
					}
					if(maxprob < dic[ii].unigram)
					{
						maxprob = dic[ii].unigram;
						maxidx = ii;
					}
				}
				//如果该词已经到结束点，节点数目恢复
				if(tree[prefather].nodeId == beginEndCode && layerCt > 1)
					nodeCt = nodeCtTmp; //the xrange node of its father

				tree[prefather].linkNum = nodeCt - nodeCtTmp;
				tree[prefather].wordFirst = xrange;
				tree[prefather].wordLast = yrange;
#ifdef STORE_MAX_PROB
				tree[prefather].maxUnigramProb = maxprob;
#else
				tree[prefather].maxUnigramProb = maxidx;
#endif
				
				//更新父节点
				prefather = father[i];
				xrange = i;
				yrange = i;
				linkNumTmp = 1;
			}
		}
		
		if(linkNumTmp)
		{
			if(linkNumTmp != yrange-xrange+1)
			{
				getch();
				return;
			}

			if(tree[prefather].nodeId == beginEndCode && layerCt > 1)
				tree[prefather].startNode = -1;
			else
				tree[prefather].startNode = nodeCt; //the xrange node of its father

			nodeCtTmp = nodeCt;

			maxprob = neg_huge;
			maxidx = -1;
			preNode = -1;
			for(ii = xrange; ii <= yrange; ii++)
			{
				if(dic[ii].code[layerCt] != preNode)
				{
					preNode = dic[ii].code[layerCt];
					nodeCt++;
					assert(nodeCt<MAX_TREE_NODE);
				}
				if(maxprob < dic[ii].unigram)
				{
					maxprob = dic[ii].unigram;
					maxidx = ii;
				}
			}

			if(tree[prefather].nodeId == beginEndCode && layerCt > 1)
				nodeCt = nodeCtTmp; //the xrange node of its father

			tree[prefather].linkNum = nodeCt - nodeCtTmp;
			tree[prefather].wordFirst = xrange;
			tree[prefather].wordLast = yrange;
#ifdef STORE_MAX_PROB
			tree[prefather].maxUnigramProb = maxprob;
#else
			tree[prefather].maxUnigramProb = maxidx;
#endif
		}

		//find node in the current layer and and add new node
		//增加下一层的新节点
		nodeCt = totalNode;
		prefather = -1;
		preNode = -1;
		for(i = 0 ; i < dictionarySize; i++)
		{
			if(isLeaf[i])
				continue;
			if(dic[i].code[layerCt-1] == beginEndCode && layerCt > 1)
			{
				isLeaf[i] = 1;
				continue;
			}
			if(prefather != father[i] || dic[i].code[layerCt] != preNode)
			{
				tree[nodeCt].nodeId = dic[i].code[layerCt];
				tree[nodeCt].toneId = dic[i].pitch[layerCt];

				prefather = father[i];
				preNode = dic[i].code[layerCt];
				//
				father[i] = nodeCt;
				nodeCt++;
				assert(nodeCt<MAX_TREE_NODE);
			}
			else
				father[i] = nodeCt - 1;
		}
		assert(nodeCt < MAX_TREE_NODE);

		leftCt = 0;
		layerCt++;
		printf("%d   %d\n", layerCt, nodeCt);
	}


	// check whether generated tree is right
	totalNode = nodeCt;
	for( i = 0; i < dictionarySize; i++)
	{
		linkNumTmp = 1;
		int firstNode = 0;
		int findSign;
		
		for(ii = 0; ii < dic[i].len; ii++)
		{
			findSign = 0;
			for(j = 0; j < linkNumTmp; j++)
			{
				if(dic[i].code[ii] == tree[firstNode+j].nodeId)
				{
					xrange = tree[firstNode+j].wordFirst;
					yrange = tree[firstNode+j].wordLast;
					if(i >= xrange && i <= yrange)
					{
						//
						if(ii == dic[i].len-1)
						{
							if(tree[firstNode+j].linkNum != 0)
							{
								printf("error1\n");
								getch();
							}
							if(tree[firstNode+j].startNode != -1)
							{
								printf("error2\n");
								getch();
							}
						}			
						
						linkNumTmp = tree[firstNode+j].linkNum;
						firstNode = tree[firstNode+j].startNode;
						findSign = 1;
						break;
					}
				}
			}
			if(findSign)
				continue;
			else
			{
				printf("tree error\n");
				getch();
				return;
			}
		}
	}
	////June 30,2000. Convert tonal FINAL to Tonaless FINAL
	for(ii = 0; ii < nodeCt; ii++)
		tree[ii].nodeId = pitchPhoneMap[tree[ii].nodeId];
	//
#ifdef STORE_MAX_PROB
	f1 = fopen("lextree.dat", "wb");
#else
	f1 = fopen("lextreeIdx.dat", "wb");
#endif
	if(f1 == NULL) 
	{
		printf("create error\n");
		getch();
		return;
	}
//	fwrite(&totalNode, sizeof(int), 1, f1);
	fwrite(tree, sizeof(CLexicalTree), nodeCt, f1);
	fclose(f1);

	printf("node number %d\n", totalNode);
	delete dic;
	delete isLeaf;
	delete father;
	delete pitchMap;
	delete pitchPhoneMap;
	getch();
}



#define MAX_CHAR_LINE	2000

void ConvertToModelString(int type)
{
	FILE	*fpt, *fptNew;
	int		i, j, len, unused, phoneNumber, phoneCodeStr[MAX_WORD_LENGTH*2], prob, occurNum;
	char	hanziStr[MAX_WORD_LENGTH*2+1], *pinyin[MAX_WORD_LENGTH+2], pitchStr[MAX_WORD_LENGTH*2+1];
	int leftPhone, rightPhone, basePhone, index;
	char basePitch, leftPitch, rightPitch;
	char	linestr[MAX_CHAR_LINE];
	
	for(i = 0; i < MAX_WORD_LENGTH + 2; i++)
		pinyin[i] = new char[MAX_WORD_LENGTH*2+1];

	//read syllable code table
	LoadPhones("sortphone62.dat");
	LoadSyllableTable("sylphone409.dat");
	//
	fpt=fopen("pitchPhoneMap.dat", "rb");
	if(fpt == NULL)
	{
		printf("pitch phone map file opened error\n");
		exit(-1);
	}
	fseek(fpt, 0, SEEK_END);
	totalPitchPhones = ftell(fpt) / (sizeof(int) + sizeof(char));
	fseek(fpt, 0, SEEK_SET);
	int		*pitchPhoneMap = new int[totalPitchPhones];
	char	*pitchMap = new char[totalPitchPhones];
	fread(pitchPhoneMap, sizeof(int), totalPitchPhones, fpt);
	fread(pitchMap, sizeof(char), totalPitchPhones, fpt);
	fclose(fpt);


	// 将词典的词的拼音转换成HMM代码串
	//fpt指定的辞典结构:词的汉字串,拼音串,概率,出现次数,词号
	//fptNew:指转换后的辞典结构,此时拼音串已经转换成相应的HMM模型编号
	//       结构:词的汉字串,模型串, 概率

	if((fptNew = fopen("lexical.txt", "wt")) == NULL)
	{
		printf("create error\n");
		exit(-1);
	}
	
	FILE* fpitch;
	if((fpitch = fopen("pitchDic.txt", "wt")) == NULL)
	{
		printf("create error\n");
		exit(-1);
	}

	fpt = fopen("gs.txt", "rt");
	if(fpt == NULL)
	{
		printf("dictionary file opened error\n");
		exit(-1);
	}
	for(i = 0; i < dictionarySize; i++)
	{
		printf("%d\n", i);
		//提取汉字、拼音和概率
		fgets(linestr, MAX_CHAR_LINE, fpt);
		int linelen = strlen(linestr);
		len = 0;
		int  pos = 0;
		j = 0;
		while(j < linelen)
		{
			if(linestr[j] == ' ' || linestr[j] == '\0' || linestr[j] == '\n')
			{
				pinyin[len][pos] = '\0';
				len++;
				pos = 0;
				//
				while(j < linelen)
				{
					j++;
					if(linestr[j] != ' ')
						break;
				}
			}
			else
			{
				pinyin[len][pos++] = linestr[j];
				j++;
			}
		}
		strcpy(hanziStr, pinyin[0]);
		prob = atoi(pinyin[len-1]);
		len = len - 2;
		for(j = 0; j < len; j++)
			strcpy(pinyin[j], pinyin[j+1]);
		//		
		phoneNumber = 0;
		if(ConvertToPhoneModelString(pinyin, len, phoneCodeStr, pitchStr, phoneNumber))
		{
			printf("error when converting phone string\n");
			getch();
			return;
		}
		//
		fprintf(fptNew, "%s %d  ", hanziStr, len *2 + 2);
		fprintf(fptNew, "%6d", totalPitchPhones-1);

		for(j = 0; j < phoneNumber; j++)
		{
			if(type == 1)
				index = FindPitchPhone210(phoneCodeStr[j], pitchStr[j], pitchPhoneMap, pitchMap);
			else if(type == 3)
				index = FindPitchPhone306(phoneCodeStr[j], pitchStr[j], pitchPhoneMap, pitchMap);
			else
			{
				printf("error submitted to converting type\n");
				return;
			}
			fprintf(fptNew, "%6d", index);
		}
		fprintf(fptNew, "%6d %d\n", totalPitchPhones - 1, prob);
		//
		fprintf(fpitch, "5 ");
		for(j = 0; j < phoneNumber; j++)
			fprintf(fpitch, "%-3d", pitchStr[j]);
		fprintf(fpitch, "%-3d\n", pitchStr[phoneNumber-1]);
	}

	fclose(fptNew);
	fclose(fpt);
	fclose(fpitch);
	for(i = 0; i < MAX_WORD_LENGTH + 2; i++)
		delete pinyin[i];
	delete pitchPhoneMap;
	delete pitchMap;
}


//
int FindPitchPhone306(int phone, char pitch, int* pitchPhStack, char* pitchStack)
{
	int i = -1;
	for(i = 0; i < totalPitchPhones; i++)
	{
		if(phone == pitchPhStack[i] && pitchStack[i] == pitch)
			break;
	}
	return i;
}
int FindPitchPhone210(int phone, char pitch, int* pitchPhStack, char* pitchStack)
{
	int i = -1;
	if(phone < TOTAL_INITIAL)
	{
		for(i = 0; i < totalPitchPhones; i++)
		{
			if(phone == pitchPhStack[i])
				break;
		}
	}
	else
	{
		for(i = 0; i < totalPitchPhones; i++)
		{
			if(phone == pitchPhStack[i] && pitchStack[i] == pitch)
				break;
		}
	}
	return i;
}
//

/*******************************************************
 * Make Known the Consont & Vowvel For a Given Syllable
*******************************************************/
int ConvertToPhoneModelString(char** pinYinString, int len, int *phoneIdx, char* pitchIdx, int &phoneNum)
{
	int		i, j, flag;
	char	pinyin[12], pitch;

	if(len >= MAX_WORD_LENGTH*2)
	{
		printf("Error!");
		exit(-1);
	}

	for(i = 0; i < len; i++)
	{
		strcpy(pinyin, pinYinString[i]);
		if(pinyin[strlen(pinyin)-1] >= '1' && pinyin[strlen(pinyin)-1] <= '5')
		{
			pitch = pinyin[strlen(pinyin)-1];
			switch(pitch)
			{
				case '1':
					pitch = 1;
					break;
				case '2':
					pitch = 2;
					break;
				case '3':
					pitch = 3;
					break;
				case '4':
					pitch = 4;
					break;
				case '5':
					pitch = 0;
					break;
				default:
					printf("pitch value if overflown\n");
					getch();
					break;
			}
			pinyin[strlen(pinyin)-1] = '\0';  
		}
		else
		{
			printf("pronunciation error: %s\n", pinyin);
			getch();
		}
	
		flag = 0;
		for(j = 0; j < TOTAL_SYLLABLES; j++)
		{
			if(stricmp(pinYinTab[j].pinYin, pinyin) == 0)
			{
				pitchIdx[phoneNum] = pitch;
				phoneIdx[phoneNum] = pinYinTab[j].shMuIndex;
				phoneNum++;
				pitchIdx[phoneNum] = pitch;
				phoneIdx[phoneNum] = pinYinTab[j].yMuIndex;
				phoneNum++;
			
				flag = 1;
				break;
			}
		}
		if(!flag)
		{
			printf("Can't find %s",pinyin);
			return -1;
		}
	}		
	return 0;
}



void ChangeUnigramToXU()
{
FILE *f1, *f2;

	typedef struct{
		char hanzi[21];
		int len;
		int code[50];
		long  unigram;
		int oldWordCode;
	}CSortedDictionary;

	long prob,beta,i,j;

	CSortedDictionary dic;

	f1 = fopen("lexicalSorted.txt", "rt");
	f2 = fopen("unigramGaotxt.lm", "rt");
	FILE*f3=fopen("lexicalSorted1.txt", "wt");//according to XU's LM
	if(f1 == NULL || f2==NULL ||f3==NULL)
	{
		printf("open dic error\n");
		return;
	}
	for(i = 0; i < dictionarySize; i++)
	{
		fscanf(f1, "%s%d", dic.hanzi, &dic.len);
		//dic[i].len = strlen(dic[i].hanzi) / 2 + 2;
		for(j = 0; j < dic.len; j++)
			fscanf(f1, "%d", &dic.code[j]);
		fscanf(f1, "%d%d", &dic.unigram, &dic.oldWordCode);
		//
		fscanf(f2,"%d%d",&prob,&beta);
		
		fprintf(f3, "  %-16s%-6d", dic.hanzi, dic.len);
		for(j = 0; j < dic.len; j++)
			fprintf(f3, "%-6d", dic.code[j]);
		fprintf(f3, " %d %d\n", prob, dic.oldWordCode);
	
	}
	fclose(f2);
	fclose(f1);
	fclose(f3);
}


void UpdateUnigram()
{
	FILE *f1, *f2;
	int i, j, nodeCt, leftCt, preNode, ii;
	unsigned char *pUnigram;

	typedef struct{
		char hanzi[21];
		int len;
		int code[50];
		long  unigram;
		int oldWordCode;
	}CSortedDictionary;

	CSortedDictionary dic;
	
	f1 = fopen("gs_80_unigram.lm", "rb");
	if(f1 == NULL)
	{
		printf("unigram file open error\n");
		return;
	}
	fseek(f1, 0L, SEEK_END);
	i = ftell(f1);
	pUnigram = new unsigned char[i];
	if(pUnigram == NULL)
	{
		printf("memory is not sufficient2\n");
		fclose(f1);
		return ;
	}
	fseek(f1, 0L, SEEK_SET);
	fread(pUnigram, sizeof(unsigned char), i, f1);
	fclose(f1);
	//
	f1 = fopen("lexicalSorted.txt", "rt");
	f2 = fopen("lexicalSortedUpdate.txt", "wt");
	if(f1 == NULL || f2 == NULL)
	{
		printf("open dic error\n");
		delete pUnigram;
		return;
	}
	for(i = 0; i < dictionarySize; i++)
	{
		fscanf(f1, "%s%d", dic.hanzi, &dic.len);
		for(j = 0; j < dic.len; j++)
			fscanf(f1, "%d", &dic.code[j]);
		fscanf(f1, "%d%d", &dic.unigram, &dic.oldWordCode);
		//update unigram with LM from zhangHong
		dic.unigram = *(long*)(pUnigram + i * 8L);

		fprintf(f2, "  %-16s%-6d", dic.hanzi, dic.len);
		for(j = 0; j < dic.len; j++)
			fprintf(f2, "%-6d", dic.code[j]);
		fprintf(f2, " %d %d\n", dic.unigram, dic.oldWordCode);		
	}
	fclose(f1);
	fclose(f2);
	delete pUnigram;
}


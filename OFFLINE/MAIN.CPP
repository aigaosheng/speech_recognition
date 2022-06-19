/**************************************************************************
Description: one pass decder
Modified by: Gao sheng
Date: Jan 20,2000
***************************************************************************/
#include	<stdlib.h>
#include	<stdio.h>
#include	<math.h>
#include	<string.h>
#include	<assert.h>
#include	<conio.h>
#include	<time.h>

#include	"include\const.h"
#include	"include\struct.h"
#include	"include\pitchExtractor.h"	
#include	"include\extern.h"
#include	"include\loaddata.h"
#include	"include\onepassSearch.h"
#include	"include\feature.h"

int totalFrames=0;

extern short		*hp_speech;
extern float		FiltCoe1[FFTLen / 2 + 1], FiltCoe2[FFTLen/2 + 1], En[FiltNum + 1];
extern short		Num[FFTLen / 2 + 1];

void	main(int  argc, char**  argv)
{
	FILE	*InputFile, *OutFile, *discriptFile, *fconfig;
	short	i, j, testCt, TestUtteranceNum;
	char	TestFileName[400], FilePathName[600], str[15],
			hanziStr[600], scriptFileName[400], waveListName[400], wavePath[400], outFileName[400];//inyinString[500], unused[40], senString[500];
	time_t	startTime, endTime;
	int noResult = 0;
	char srcSyl[20];
	short int		syllableLength;

	//
	if(argc != 2)
	{
		printf("This execution needs a configure file\n");
		getch();
		exit(-1);
	}
	//
	if((fconfig = fopen(argv[1], "rt")) == NULL)
	{
		printf("configure file open error\n");
		getch();
		exit(-1);
	}
	fscanf(fconfig, "%s%s%s%s", scriptFileName, waveListName, wavePath, outFileName);
	fclose(fconfig);

	if((InputFile = fopen(waveListName, "rt")) == NULL)
	{
		printf("The description file of test speech Opened  file error!\n");
		getch();
		exit(-1);
	}
	
	if((OutFile = fopen(outFileName, "wt")) == NULL)
	{
		printf("Out file Opened  file error!\n");
		getch();
		exit(-1);
	}

	//read dercription file 
	if((discriptFile = fopen(scriptFileName, "rt")) == NULL)
	{
		printf("Discription file open error\n");
		getch();
		exit(-1);
	}
	//
	// Calculate window function
	GenHamming();
	InitFilt(FiltCoe1, FiltCoe2, Num);
	time(&startTime);
	//Load Model parameters
	if(LoadData())
	{
		printf("Load data error\n");
		getch();
		exit(-1);
	}
	time(&endTime);
	//

	fprintf(OutFile, "Execute Time : %8f\n", difftime(endTime, startTime));

	InitPitchExtractor();	

	//allocate memory to store primitive speech data
 	hp_speech = new short[MAX_SPEECH_LENGTH * hFrameLen];
	assert(hp_speech);

	//Process all utterances sequenctly 
	fscanf(InputFile, "%s", str);
	TestUtteranceNum = atoi(str);
	//record start Time 
	time(&startTime);
	//
	int totalSyl = 0;
	//Initialize the dis-lm cache
	memset(pLmCacheHead, 0, MAX_TREE_NODE*2);
	memset(lmCacheFull, 0, MAX_TREE_NODE);
	//
	for(testCt = 0; testCt < TestUtteranceNum; testCt++)
	//for(testCt = 0; testCt < 1; testCt++)
	{
		fscanf(InputFile, "%s", TestFileName);
		strcpy(FilePathName, wavePath);
		strcat(FilePathName, TestFileName);
		//strcpy(FilePathName, TestFileName);

		fscanf(discriptFile, "%s", hanziStr);
		syllableLength = strlen(hanziStr) / 2;
		if(syllableLength > MAX_NUM_WORD)
		{
			printf("array is too small\n");
			getch();
			exit(-1);
		}
		//fprintf(OutFile, "%s\n%s\n", TestFileName, hanziStr);
		for(j = 0; j < syllableLength; j++)
			fscanf(discriptFile, "%s", srcSyl);

		//extract the features
		FeatureExtract(FilePathName);	
	//	continue;

		// Begin to recognize the utterance 
		OnepassSearchEngine();

		char sentence[200];
		char chtmp[30];
		strcpy(sentence, "");

		if(senLen > 0)
		for(i = senLen-1; i >= 0 ; i--)
		{
			for(j = 0; j < MAX_WORD_LENGTH*2; j++)
				chtmp[j] = pHanzi[wordSeq[i]*MAX_WORD_LENGTH*2+j];
			strcat(sentence, chtmp);
			//strcat(sentence, "/");
			//fprintf(OutFile, "%d %d %d,", wordSeq[i], timeSeq[i], posSeq[i]);
		}
		fprintf(OutFile, "%s\n", sentence);
		printf("%d %s\n%s\n", testCt, TestFileName, sentence);
	}
	//
	time(&endTime);
	//

	fprintf(OutFile, "Execute Time : %8f\n", difftime(endTime, startTime));

	fclose(OutFile);
	fclose(InputFile);
	fclose(discriptFile);
	delete	hp_speech;
	ReleaseMemory();

	printf("%d\n", totalFrames);
}

int ExtractPinyin(char* stream, char **pinyin)
{
	int sylCt, posCt, k, len;

	sylCt = 0; //counter of syllable number
	posCt = 0;
	len = strlen(stream);
	for(k = 0; k < len; k++)
	{
		if(stream[k] == '-')
		{
			pinyin[sylCt][posCt] = '\0';
			sylCt++;
			posCt = 0;
			if(sylCt > MAX_NUM_WORD)
			{
				printf("syllable number in the sentence is over maxial value\n");
				return 0;
			}
		}
		else
		{
			pinyin[sylCt][posCt] = stream[k];
			posCt++;
		}
	}
	pinyin[sylCt][posCt] = '\0';
	return sylCt+1;
}





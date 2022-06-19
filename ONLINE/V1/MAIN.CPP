#include	<stdio.h>
#include	<stdlib.h>
#include	<conio.h>
#include "recognize.h"
extern int RecogThreadProc(void);
#include "stdafx.h"
#include	"const.h"
#include	"global.h"
#include	"modemanager.h"

//#define DEBUG_HASH_EFFICIENT
#ifdef DEBUG_HASH_EFFICIENT
extern int intraFound, intraTotal, intraCt;
extern int interFound, interTotal, interCt;
extern int	wordFound, wordTotal, wordCt;
extern int maxdepth[3];
#endif

//#define	DEBUG_LM_CACHE
#ifdef	DEBUG_LM_CACHE
extern int findCt, totalCt,totalcycle,interlmct,interfind,intertotal;
#endif
//extern int small,high[2],smallpath,highpath[2],levelnum[3][5],smalllevel[3][5];
//extern int totalGau, comGau;
extern int totalFrame;


void	main()
{
	SrStartDragonVoice("MALE");
	SrLoadDictationMode();

	char name[200], fileName[500];
	int num; 
	FILE *fp;

//	fp = fopen("e:\\mywork\\decoder\\onepassApiPlat\\profileRealtime\\testmale.txt", "rt");
	fp = fopen("e:\\mywork\\decoder\\onepassApiPlat\\testPlat\\offtestscript.txt", "rt");
	if(fp == NULL)
	{
		printf("testmale.txt is not found");
		return;
	}
	fscanf(fp, "%d", &num);
	int nTime = GetTickCount(); 
	for(int i = 0; i < num; i++)
	{
		fscanf(fp, "%s", fileName);
		SrOfflineCepTest(fileName);//
		RecogThreadProc();
		printf("\n");
	}
	fclose(fp);
	int m_time = GetTickCount() - nTime;
	printf("%d\n", m_time);

	//
	SrExitDragonVoice();
	//

#ifdef DEBUG_HASH_EFFICIENT
	//FILE *ff=fopen("list.txt", "wt");
	//int nullct=0;	
	printf("intra Total: %d, found: %d, %f, %f\n", intraTotal, intraFound, intraFound*100.0/intraTotal, (float)(intraCt)/intraFound);	
	printf("inter Total: %d, found: %d, %f, %f\n", interTotal, interFound, interFound*100.0/interTotal, (float)(interCt)/interFound);	
	printf("word Total: %d, found: %d, %f, %f\n", wordTotal, wordFound, wordFound*100.0/wordTotal, (float)(wordCt)/wordFound);	
	printf("max depth in hash: %d, %d, %d\n", maxdepth[0], maxdepth[1],maxdepth[2]);
	//fprintf(ff, "intra Total: %d, found: %d, %f, %f\n", intraTotal, intraFound, intraFound*100.0/intraTotal, (float)(intraCt)/intraFound);	
	//fprintf(ff, "inter Total: %d, found: %d, %f, %f\n", interTotal, interFound, interFound*100.0/interTotal, (float)(interCt)/interFound);	
	//fprintf(ff, "word Total: %d, found: %d, %f, %f\n", wordTotal, wordFound, wordFound*100.0/wordTotal, (float)(wordCt)/wordFound);	
	//fclose(ff);
	//
#endif
#ifdef	DEBUG_LM_CACHE
	printf("%d, %d, %d, %f, %f, %d, %d,%f\n", findCt, totalCt, totalcycle,(float)findCt/totalcycle,(float)totalCt/totalcycle, interlmct,interfind,(float)intertotal/(interlmct+interfind));
#endif
	
	/*/FILE *fdbg=fopen("cache.txt", "wt");
	int ttt=0;
	for(i = 0; i < MAX_TREE_NODE; i++)
		//fprintf(fdbg,"%d\n", g_pLextree->m_LM->lmCacheFull[i]);
		if(g_pLextree->m_LM->lmCacheFull[i] || g_pLextree->m_LM->pLmCacheHead[i] > 0)
		{
			ttt++;
	//		fprintf(fdbg, "1\n");
		}
	//	else
	//		fprintf(fdbg, "0\n");
	printf("%d %f\n", ttt, (float)ttt/MAX_TREE_NODE);
	//fclose(fdbg);
	*/
/*
	printf("%d, %d, %f\n %d, %d, %f\n%d, %d, %f\n",small,smallpath,(float)small/smallpath,high[0],highpath[0],(float)high[0]/(highpath[0]+highpath[1]),
		high[1],highpath[1],(float)high[1]/(highpath[0]+highpath[1]));
	for(i=0;i<3;i++)
	{	
		for(int j=0;j<5;j++)
			printf("%d, ",levelnum[i][j]);
		printf("\n");
		for( j=0;j<5;j++)
			printf("%d, ",smalllevel[i][j]);
		printf("\n\n");
	}
	
	//printf("%d, %d\n", totalGau, comGau);
*/	
	printf("%d\n", totalFrame);
}


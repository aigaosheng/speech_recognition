// AM.cpp: implementation of the CAM class.
//
//////////////////////////////////////////////////////////////////////
#include	<conio.h>
#include	"stdafx.h"
#include	"struct.h"
#include	"const.h"
#include	"AM.h"
#include	"global.h"

#define	FAST_GAUSSIAN
#define	USE_SSE  //如果不用SSE汇编优化，仅对PIII使用
//#define USE_AM_PREDICT

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const double logbase = 20000.05; //10*log(1.000005);
extern char	amTopic[50];
//const double PAI2 = 8 * atan(1.0);		//2 * Pai
//int high[2]={0,0},small=0,smallpath=0,highpath[2]={0,0},levelnum[3][5]={{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0}}, smalllevel[3][5]={{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0}};
//int totalGau=0, comGau=0;

CAM::CAM()
{
	m_pTriphoneMappingInitial = NULL;
	m_pTriphoneMappingFinal = NULL;
	m_pMapping = NULL;
	m_pOutput = NULL;

//
	TCHAR lpszFileName[MAX_PATH];
	DWORD dwSize = GetPrivateProfileString(amTopic, "TRIPHONE_NUM", NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);
	m_nTotalTriphone = atoi(lpszFileName);
	m_triSilenceCode = m_nTotalTriphone - 1;

	dwSize = GetPrivateProfileString(amTopic, "PDF_NUM", NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);
	m_nTotalOutput = atoi(lpszFileName);

//
	m_pHmm = new HMM[m_nTotalTriphone];//TRI_NO_HMM];
	memset(m_pHmm, 0, sizeof(HMM) * m_nTotalTriphone);//TRI_NO_HMM);

	//m_pObserveBuffer = NULL;
#ifdef USE_SSE
	m_pVector = (__m128*)VirtualAlloc(NULL,sizeof(__m128) * NO_DIM_ALIGNED, MEM_COMMIT, PAGE_READWRITE);
#endif

	m_pObserveBuffer = new long[m_nTotalOutput];//NO_OUTPUT];
	m_time = 0;
}

CAM::~CAM()
{
//	printf ("Time used by GetObserveProb is %d\n", m_time);

	if (m_pTriphoneMappingInitial != NULL)
		VirtualFree(m_pTriphoneMappingInitial, 0, MEM_RELEASE);
	if (m_pMapping != NULL)
		VirtualFree(m_pMapping, 0, MEM_RELEASE);
	if (m_pOutput != NULL)
		VirtualFree(m_pOutput, 0, MEM_RELEASE);


	//for (int i = 0; i< TRI_NO_HMM; i++){
	for (int i = 0; i< m_nTotalTriphone; i++){
		if (m_pHmm[i].nj != NULL)
			delete []m_pHmm[i].nj;
		if (m_pHmm[i].indj)
			delete []m_pHmm[i].indj;
		if (m_pHmm[i].aLinks)
			delete []m_pHmm[i].aLinks;
		if (m_pHmm[i].bLinks)
			delete []m_pHmm[i].bLinks;
		if (m_pHmm[i].OutputID)
			delete []m_pHmm[i].OutputID;
		if (m_pHmm[i].A)
			delete []m_pHmm[i].A;
	}
	if(m_pHmm != NULL)
		delete []m_pHmm;

	if (m_pObserveBuffer != NULL)
		delete []m_pObserveBuffer;

#ifdef USE_SSE
	VirtualFree(m_pVector, 0, MEM_RELEASE);
#endif
	//以下由高声编辑
	delete	[]m_pShortListNum;
	delete	[]m_pShortListOffset;
	delete	[]m_pShortList;
	delete	[]m_pClusterMean;
	delete	[]m_pClusterVar;
}

BOOL CAM::Init()
{
	BOOL ret = LoadMapping();
	ASSERT(ret);

	ret = LoadTriphoneMapping();
	ASSERT(ret);

	ret = LoadHmm();
	ASSERT(ret);

	ret = LoadOutput();
	ASSERT(ret);
	//以下由高升编辑
	ret = LoadShortList();
	ASSERT(ret);
	//
	return TRUE;
}

BOOL CAM::LoadMapping()
{
	//获取文件路径
	TCHAR lpszFileName[MAX_PATH];
	DWORD dwSize = GetPrivateProfileString(amTopic, "Mapping", NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);

	//计算扇区的大小
	DWORD dwSecPerCluster, dwBytePerSector, dwNumFreeCluster, dwNumTotalCluster;
	if (!GetDiskFreeSpace(NULL, &dwSecPerCluster, &dwBytePerSector, &dwNumFreeCluster, 
		&dwNumTotalCluster)){
		TRACE1("There is error when GetDiskFreeSpace(), Error num = %d\n", GetLastError());
		return FALSE;
	}


	//打开文件并获取文件大小
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		return FALSE;
	}

	dwSize = GetFileSize(hFile, NULL);
	ASSERT(dwSize != INVALID_FILE_SIZE);

	//分配内存
	m_pMapping = (PBYTE)VirtualAlloc(NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (m_pMapping == NULL){
		TRACE("There is not enough memory!\n");
		CloseHandle(hFile);
		return FALSE;
	}

	//计算文件所占扇区个数
	int	nBuffer = dwSize / dwBytePerSector;
	if (dwSize % dwBytePerSector)
		nBuffer++;

	//读取到缓冲区 
	DWORD dwNumByteRead;
	if (!ReadFile(hFile, (LPVOID)m_pMapping, dwBytePerSector * nBuffer, &dwNumByteRead, NULL)){
		TRACE1("Error when read Bigram Index file, Error Num = %d\n", GetLastError());
		CloseHandle(hFile);
		VirtualFree(m_pMapping, 0, MEM_RELEASE);
		return FALSE;
	}

	//关闭文件
	CloseHandle(hFile);
	
	/*/计算每个triphone的起始编码以及每个base phone对应的最多triphone个数
	int previous;
	previous = m_pMapping[0];
	baseStartTricode[previous] = 0;
	for(int kk=0;kk<TOTAL_TRIPHONES;kk++)
	{
		if(m_pMapping[kk] != previous)
		{
			baseStartTricode[m_pMapping[kk]] = kk;
			previous = m_pMapping[kk];
		}
	}
	maxTriphoneNum = 0;
	for(kk = 0; kk < TOTAL_PHONES - 1; kk++)
		if(baseStartTricode[kk+1] - baseStartTricode[kk] > maxTriphoneNum)
			maxTriphoneNum = baseStartTricode[kk+1]-baseStartTricode[kk];
	*/
	return TRUE;
}

#ifdef USE_SSE
BOOL CAM::LoadOutput()
{
	//计算扇区的大小
	DWORD dwSecPerCluster, dwBytePerSector, dwNumFreeCluster, dwNumTotalCluster;
	if (!GetDiskFreeSpace(NULL, &dwSecPerCluster, &dwBytePerSector, &dwNumFreeCluster, 
		&dwNumTotalCluster)){
		TRACE1("There is error when GetDiskFreeSpace(), Error num = %d\n", GetLastError());
		return FALSE;
	}

	//获取文件路径
	TCHAR lpszFileName[MAX_PATH];
	DWORD dwSize = GetPrivateProfileString(amTopic, "OutputList", NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);

	//打开文件并获取文件大小
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, 
							  NULL, OPEN_EXISTING,
							  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, 
							  NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		return FALSE;
	}

	dwSize = GetFileSize(hFile, NULL);
	ASSERT(dwSize != INVALID_FILE_SIZE);
	ASSERT(dwSize == sizeof(OUTPUT_ALIGNED) * m_nTotalOutput);//NO_OUTPUT);

	//分配内存
	m_pOutput = (OUTPUT_ALIGNED *)VirtualAlloc(NULL, 
				sizeof(OUTPUT_ALIGNED) * m_nTotalOutput, MEM_COMMIT, PAGE_READWRITE);
	if (m_pOutput == NULL){
		TRACE("There is not enough memory!\n");
		CloseHandle(hFile);
		return FALSE;
	}

	//计算文件所占扇区个数
	int	nBuffer = dwSize / dwBytePerSector;
	if (dwSize % dwBytePerSector)
		nBuffer++;

	//读取到缓冲区 
	DWORD dwNumByteRead;
	if (!ReadFile(hFile, (LPVOID)m_pOutput, dwBytePerSector * nBuffer, &dwNumByteRead, NULL)){
		TRACE1("Error when read Bigram Index file, Error Num = %d\n", GetLastError());
		CloseHandle(hFile);
		VirtualFree((LPVOID)m_pOutput, 0, MEM_RELEASE);
		return FALSE;
	}

	//关闭文件
	CloseHandle(hFile);

	return TRUE;
}
#else
//不使用SSE优化
BOOL CAM::LoadOutput()
{
	//计算扇区的大小
	DWORD dwSecPerCluster, dwBytePerSector, dwNumFreeCluster, dwNumTotalCluster;
	if (!GetDiskFreeSpace(NULL, &dwSecPerCluster, &dwBytePerSector, &dwNumFreeCluster, 
		&dwNumTotalCluster)){
		TRACE1("There is error when GetDiskFreeSpace(), Error num = %d\n", GetLastError());
		return FALSE;
	}

	//获取文件路径
	TCHAR lpszFileName[MAX_PATH];
	DWORD dwSize = GetPrivateProfileString(amTopic, "OutputList", NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);

	//打开文件并获取文件大小
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, 
							  NULL, OPEN_EXISTING,
							  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, 
							  NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		return FALSE;
	}

	dwSize = GetFileSize(hFile, NULL);
	ASSERT(dwSize != INVALID_FILE_SIZE);
	ASSERT(dwSize == sizeof(OUTPUT_ALIGNED) * m_nTotalOutput);//NO_OUTPUT);

	//分配内存
	m_pOutput = (OUTPUT_ALIGNED *)VirtualAlloc(NULL, 
				sizeof(OUTPUT_ALIGNED) * m_nTotalOutput, MEM_COMMIT, PAGE_READWRITE);
	if (m_pOutput == NULL){
		TRACE("There is not enough memory!\n");
		CloseHandle(hFile);
		return FALSE;
	}

	//计算文件所占扇区个数
	int	nBuffer = dwSize / dwBytePerSector;
	if (dwSize % dwBytePerSector)
		nBuffer++;

	//读取到缓冲区 
	DWORD dwNumByteRead;
	if (!ReadFile(hFile, (LPVOID)m_pOutput, dwBytePerSector * nBuffer, &dwNumByteRead, NULL)){
		TRACE1("Error when read Bigram Index file, Error Num = %d\n", GetLastError());
		CloseHandle(hFile);
		VirtualFree((LPVOID)m_pOutput, 0, MEM_RELEASE);
		return FALSE;
	}

	//关闭文件
	CloseHandle(hFile);

	return TRUE;
}


#endif 



BOOL CAM::LoadTriphoneMapping()
{
	//计算扇区的大小
	DWORD dwSecPerCluster, dwBytePerSector, dwNumFreeCluster, dwNumTotalCluster;
	if (!GetDiskFreeSpace(NULL, &dwSecPerCluster, &dwBytePerSector, &dwNumFreeCluster, 
		&dwNumTotalCluster)){
		TRACE1("There is error when GetDiskFreeSpace(), Error num = %d\n", GetLastError());
		return FALSE;
	}

	//获取文件路径
	TCHAR lpszFileName[MAX_PATH];
	DWORD dwSize = GetPrivateProfileString(amTopic, "TriphoneList", NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);

	//打开文件并获取文件大小
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, 
							  NULL,	OPEN_EXISTING, 
							  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, 
							  NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		return FALSE;
	}

	dwSize = GetFileSize(hFile, NULL);
	ASSERT(dwSize != INVALID_FILE_SIZE);
	//ASSERT(dwSize % sizeof(TRIPHONEMAPPING) == 0);
	//ASSERT(dwSize / sizeof(short)== TOTAL_INITIAL * (TOTAL_FINAL + 1) * TOTAL_FINAL
	//				+ TOTAL_FINAL * TOTAL_INITIAL * (TOTAL_INITIAL + 1) * 5 * 6 * 6);

	// 分配内存，将m_pTriphoneMappingIndex指向次内存块的开始位置，
	// 实际上，m_pTriphoneMappingIndex不需要占用如此大的内存，
	// m_pTriphoneMapping指向TriphoneMapping的开始位置，
	// 也就是m_pTriphoneMappingIndex结束后的位置。
	m_pTriphoneMappingInitial = (short *)VirtualAlloc(NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (m_pTriphoneMappingInitial == NULL){
		TRACE("There is not enough memory!\n");
		CloseHandle(hFile);
		return FALSE;
	}

	//计算文件所占扇区个数
	int	nBuffer = dwSize / dwBytePerSector;
	if (dwSize % dwBytePerSector)
		nBuffer++;


	//读取到缓冲区 
	DWORD dwNumByteRead;
	if (!ReadFile(hFile, (LPVOID)m_pTriphoneMappingInitial, 
		dwBytePerSector * nBuffer, &dwNumByteRead, NULL)){
		TRACE1("Error when read Bigram Index file, Error Num = %d\n", GetLastError());
		CloseHandle(hFile);
		VirtualFree((LPVOID)m_pTriphoneMappingInitial, 0, MEM_RELEASE);
		return FALSE;
	}

	//关闭文件
	CloseHandle(hFile);

	// 把m_pTriphoneMapping的指针指向m_pTriphoneMapping的开始位置
	m_pTriphoneMappingFinal = m_pTriphoneMappingInitial  + TOTAL_INITIAL * (TOTAL_FINAL + 1) * TOTAL_FINAL;

	return TRUE;
}

BOOL CAM::LoadHmm()
{
	//获取文件路径
	TCHAR lpszFileName[MAX_PATH];
	DWORD dwSize = GetPrivateProfileString(amTopic, "ModelList", NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);

	int i,n_State,n_Out,n_Trans;
	FILE *HmmFile;

	if((HmmFile=fopen(lpszFileName, "rb"))==NULL)
	{
		printf("\nCan't open hmmList.dat!");
		return FALSE;
	}

//	for(i=0;i<TRI_NO_HMM;i++)
	for(i=0;i<m_nTotalTriphone;i++)
	{
		fread(&(m_pHmm[i].No_State),sizeof(int),1,HmmFile);
		fread(&(m_pHmm[i].No_Out),sizeof(int),1,HmmFile);
		fread(&(m_pHmm[i].No_Trans),sizeof(int),1,HmmFile);

		n_State=m_pHmm[i].No_State;
		n_Out=m_pHmm[i].No_Out;
		n_Trans=m_pHmm[i].No_Trans;

		m_pHmm[i].nj=new int[n_State];
		fread(m_pHmm[i].nj,sizeof(int),n_State,HmmFile);

		m_pHmm[i].indj=new int[n_State*n_State];
		fread(m_pHmm[i].indj,sizeof(int),n_State*n_State,HmmFile);

		m_pHmm[i].aLinks=new int[n_State*n_State];
		fread(m_pHmm[i].aLinks,sizeof(int),n_State*n_State,HmmFile);

		m_pHmm[i].bLinks=new int[n_State*n_State];
		fread(m_pHmm[i].bLinks,sizeof(int),n_State*n_State,HmmFile);

		m_pHmm[i].A=new float[n_Trans];
		fread(m_pHmm[i].A,sizeof(float),n_Trans,HmmFile);
		//预先计算对数值
		for(int ii = 0; ii < n_Trans; ii++)
			m_pHmm[i].A[ii] = log10(m_pHmm[i].A[ii]) * logbase;
		//

		m_pHmm[i].OutputID = new int[n_Out];
		if(m_pHmm[i].OutputID==NULL)
		{
			printf("\nAllocating m_pHmm[i].OutputID Error!");
			return FALSE;
		}
		fread(m_pHmm[i].OutputID,sizeof(int),n_Out,HmmFile);
	}

	fclose(HmmFile);
	return TRUE;
}

//增加查找码字
void CAM::ResetObserveBuffer(float *pVector)
{
/*
	if (m_pObserveBuffer == NULL)
		m_pObserveBuffer = new long[m_nTotalOutput];

	for (int i = 0; i < m_nTotalOutput; i++)
		m_pObserveBuffer[i] = NEG_HUGE;
*/
//	memcpy(m_pVector, pVector, sizeof(float) * NO_DIM);
	//以下由高升编辑
#ifdef FAST_GAUSSIAN 

#ifdef USE_AM_PREDICT
	if(m_codeWord < 0)
	{
		memcpy(m_pVector, pVector, sizeof(float) * NO_DIM);
		m_codeWord = FindCodebook(pVector);
	}
	//预测
	BOOL ret = g_pAudio->ReadFIFONoshift(nextVector);
	m_nextCodeWord = -1;
	if(ret == FIFO_OPERATION_NORMAL)
		m_nextCodeWord = FindCodebook(nextVector);
#else
	memcpy(m_pVector, pVector, sizeof(float) * NO_DIM);
	m_codeWord = FindCodebook(pVector);
#endif

#else
	memcpy(m_pVector, pVector, sizeof(float) * NO_DIM);

#endif

}

void CAM::ResetAmCacheBuffer(void)
{
	for (int i = 0; i < m_nTotalOutput; i++)
		m_pObserveBuffer[i] = NEG_HUGE;
#ifdef USE_AM_PREDICT
	memcpy(m_pVector, nextVector, sizeof(float) * NO_DIM);
	m_codeWord = m_nextCodeWord;
#endif
}

long CAM::GetObserveProbOld(WORD nModel, WORD iState, WORD jState)
{
#ifdef USE_SSE

	double TransLikelihood;
	int ii,nOut, j;
	float zhishu;

	TransLikelihood = 0;

	nOut = m_pHmm[nModel].bLinks[iState*m_pHmm[nModel].No_State+jState];
	nOut = m_pHmm[nModel].OutputID[nOut];
	
	ASSERT(nOut < m_nTotalOutput);
	if(m_pObserveBuffer[nOut] != NEG_HUGE)
		return m_pObserveBuffer[nOut];		

	LPOUTPUT_ALIGNED pOutput = m_pOutput + nOut;
	//以下注释的部分为最原始的程序，没有经过任何改动。
/*	for(ii = 0; ii < NO_MIX; ii++)
	{
		zhishu = 0;
		xishu = PAI2;

		for(j = 0; j < NO_DIM; j++)
		{

//   	第14维为声调，所以在无声调时屏蔽掉
//#ifndef USE_PITCH			
//			if((j + 1) % 14 == 0)
//				continue;
//#endif			

			temp = ii * NO_DIM + j;
			fenzi = m_pVector[j] - pOutput->Means[temp];
			fenzi *= fenzi;

			fenmu = pOutput->Variance[temp];
			zhishu += fenzi/fenmu;
			xishu *= fenmu;
		}
	
		fenzi = exp(-0.5 * zhishu);
		fenmu = sqrt(xishu);

		TransLikelihood += pOutput->CoeMix[ii] * (fenzi / fenmu);
	}
	long x;
	if(TransLikelihood != 0.0)
		x = (long)(log10(TransLikelihood) / logbase);
	else
		x = NEG_HUGE - 1;

	m_pObserveBuffer[nOut] = x;
*/
	for (ii = 0; ii < NO_MIX; ii++){

		zhishu = 0;		//此行是新程序都需要的公用部分。

		//从此处到 _mm_store_ss(&zhishu, vecZhishu)为使用SSE指令优化的部分。
		__m128 *pMeans = pOutput->Means[ii];
		__m128 *pVariance = pOutput->Variance[ii];

		__m128 vecTemp;
		__m128 vecZhishu = _mm_setzero_ps();
		for (j = 0; j < NO_DIM_ALIGNED; j ++){
			vecTemp = _mm_sub_ps(m_pVector[j], pMeans[j]);
			vecTemp = _mm_mul_ps(vecTemp, vecTemp);
			vecTemp = _mm_mul_ps(vecTemp, pVariance[j]);

			vecZhishu = _mm_add_ps(vecTemp, vecZhishu);
		}

		vecTemp = _mm_shuffle_ps(vecTemp, vecZhishu, 0x40);
		vecZhishu = _mm_add_ps(vecTemp, vecZhishu);
		vecTemp = _mm_shuffle_ps(vecTemp, vecZhishu, 0x30);
		vecZhishu = _mm_add_ps(vecTemp, vecZhishu);
		vecZhishu = _mm_shuffle_ps(vecZhishu, vecZhishu, 2);

		_mm_store_ss(&zhishu, vecZhishu);

		//以下的注释为不使用SSE优化的部分。
		//注意：由于数据类型已和原来的不同，因此，暂时不能运行。

/*		float *pMeans = pOutput->Means + ii * NO_DIM;
		float *pVariance = pOutput->Variance  + ii * NO_DIM;
		for (j = 0; j < NO_DIM; j ++){

			//temp = ii * NO_DIM + j;
			//fenzi = m_pVector[j] - pOutput->Means[temp];
			fenzi = m_pVector[j] - pMeans[j];
			fenzi *= fenzi;

			//fenzi *= pOutput->Variance[temp];
			fenzi *= pVariance[j];

			zhishu += fenzi;
		}
*/
		TransLikelihood += pOutput->CoeMix[ii] * exp(zhishu);
	}
	long x;
	if (TransLikelihood != 0.0)
		//x = (long)(log10(TransLikelihood) / logbase);
		x = (long)(log10(TransLikelihood) * logbase);
	else
		x = GAUSSIAN_BOTTOM;//NEG_HUGE - 1;

	m_pObserveBuffer[nOut] = x;
	return x;  
#endif
	return 0;
}

//以下由高声编辑，采用高斯快速算法
long CAM::GetObserveProb(WORD nModel, WORD iState, WORD jState, short vqCode)
{
	double TransLikelihood;
	int ii,nOut, j;
	float zhishu, fenzi;

//
//	totalGau++;
//
	TransLikelihood = 0;

	nOut = m_pHmm[nModel].bLinks[iState*m_pHmm[nModel].No_State+jState];
	nOut = m_pHmm[nModel].OutputID[nOut];
	
	ASSERT(nOut < m_nTotalOutput);
	if(m_pObserveBuffer[nOut] != NEG_HUGE)
		return m_pObserveBuffer[nOut];		
//
//	comGau++;
//
	LPOUTPUT_ALIGNED pOutput = m_pOutput + nOut;

	//以下由高升编辑
	int listNum, offset, index;
	listNum = m_pShortListNum[vqCode*m_nTotalOutput+nOut];
	offset = m_pShortListOffset[vqCode*m_nTotalOutput+nOut];

	//以下注释的部分为最原始的程序，没有经过任何改动。
/*	for(ii = 0; ii < NO_MIX; ii++)
	{
		zhishu = 0;
		xishu = PAI2;

		for(j = 0; j < NO_DIM; j++)
		{

//   	第14维为声调，所以在无声调时屏蔽掉
//#ifndef USE_PITCH			
//			if((j + 1) % 14 == 0)
//				continue;
//#endif			

			temp = ii * NO_DIM + j;
			fenzi = m_pVector[j] - pOutput->Means[temp];
			fenzi *= fenzi;

			fenmu = pOutput->Variance[temp];
			zhishu += fenzi/fenmu;
			xishu *= fenmu;
		}
	
		fenzi = exp(-0.5 * zhishu);
		fenmu = sqrt(xishu);

		TransLikelihood += pOutput->CoeMix[ii] * (fenzi / fenmu);
	}
	long x;
	if(TransLikelihood != 0.0)
		x = (long)(log10(TransLikelihood) / logbase);
	else
		x = NEG_HUGE - 1;

	m_pObserveBuffer[nOut] = x;
*/
	for (ii = 0; ii < listNum; ii++){

		zhishu = 0;		//此行是新程序都需要的公用部分。

		index = m_pShortList[offset+ii]; //高声编辑

#ifdef USE_SSE
		//从此处到 _mm_store_ss(&zhishu, vecZhishu)为使用SSE指令优化的部分。
		__m128 *pMeans = pOutput->Means[index];//ii];
		__m128 *pVariance = pOutput->Variance[index];//[ii];

		__m128 vecTemp;
		__m128 vecZhishu = _mm_setzero_ps();
		for (j = 0; j < NO_DIM_ALIGNED; j ++){
			vecTemp = _mm_sub_ps(m_pVector[j], pMeans[j]);
			vecTemp = _mm_mul_ps(vecTemp, vecTemp);
			vecTemp = _mm_mul_ps(vecTemp, pVariance[j]);

			vecZhishu = _mm_add_ps(vecTemp, vecZhishu);
		}

		vecTemp = _mm_shuffle_ps(vecTemp, vecZhishu, 0x40);
		vecZhishu = _mm_add_ps(vecTemp, vecZhishu);
		vecTemp = _mm_shuffle_ps(vecTemp, vecZhishu, 0x30);
		vecZhishu = _mm_add_ps(vecTemp, vecZhishu);
		vecZhishu = _mm_shuffle_ps(vecZhishu, vecZhishu, 2);

		_mm_store_ss(&zhishu, vecZhishu);
#else
		//以下的注释为不使用SSE优化的部分。
		//注意：由于数据类型已和原来的不同，因此，暂时不能运行。

		float *pMeans = pOutput->Means[index];
		float *pVariance = pOutput->Variance[index];
		for (j = 0; j < NO_DIM; j ++){

			//temp = ii * NO_DIM + j;
			//fenzi = m_pVector[j] - pOutput->Means[temp];
			fenzi = m_pVector[j] - pMeans[j];
			fenzi *= fenzi;

			//fenzi *= pOutput->Variance[temp];
			fenzi *= pVariance[j];

			zhishu += fenzi;
		}
#endif
		//TransLikelihood += pOutput->CoeMix[ii] * exp(zhishu);
		TransLikelihood += pOutput->CoeMix[index] * exp(zhishu);
	}
	long x;
	if (TransLikelihood != 0.0)
		x = (long)(log10(TransLikelihood) * logbase);
	else
		x = GAUSSIAN_BOTTOM;//NEG_HUGE - 1;

	m_pObserveBuffer[nOut] = x;

	return x;  
}


long CAM::GetTranslateProb(WORD nModel, WORD nState1, WORD nState2)
{
	int nTrans;
	nTrans=m_pHmm[nModel].aLinks[nState1 * m_pHmm[nModel].No_State+nState2];
	//long x =  (long)(log10(m_pHmm[nModel].A[nTrans]) / logbase);
	long x =  (long)(m_pHmm[nModel].A[nTrans]);

	return x;
}

void CAM::ExpandNode(PHONENODE & SeedNode)
{
	PHONENODE	nodeTmp;
	BYTE i_s = SeedNode.tokenActive;
	DWORD i_g = SeedNode.grammarNode; 
	short phnidx = SeedNode.modelId;			

//int path=0;

  	for(int j = 0; j < m_pHmm[phnidx].nj[i_s]; j++)
	{			  
		WORD j_s = m_pHmm[phnidx].indj[i_s * m_pHmm[phnidx].No_State + j]; //next HMM state

		long iaal = GetTranslateProb(phnidx, i_s, j_s);

//		DWORD nTime = GetTickCount(); //计算高斯所占用的时间
#ifdef FAST_GAUSSIAN 
		long ibbl = GetObserveProb(phnidx, i_s, j_s, m_codeWord);
#else
		long ibbl = GetObserveProbOld(phnidx, i_s, j_s);
#endif
//		m_time += GetTickCount() - nTime;

		long probAm = SeedNode.amScore + iaal + ibbl;
		long probLm = SeedNode.lmScore;

		//Check whether readily extended path exist
		nodeTmp.amScore = probAm;
		nodeTmp.lmScore = probLm;
		nodeTmp.grammarNode = i_g;
		nodeTmp.historyWord1 = SeedNode.historyWord1;
		nodeTmp.historyWord2 = SeedNode.historyWord2;
		nodeTmp.lmLookaheadScore = SeedNode.lmLookaheadScore;
		nodeTmp.modelId = phnidx;
		nodeTmp.preWordPos = SeedNode.preWordPos;
		nodeTmp.startTime = SeedNode.startTime;
		nodeTmp.wordDuration = SeedNode.wordDuration + 1;
		nodeTmp.tokenActive = j_s;//j_psf;
		nodeTmp.leftTriphone = SeedNode.leftTriphone;
		nodeTmp.leftTone = SeedNode.leftTone;
		nodeTmp.nextToken = -1;
		if(ACOUSTIC_WEIGHT * nodeTmp.amScore + LANGUAGE_WEIGHT * nodeTmp.lmScore >= g_pPath->m_lProbMax - maxOffsetAm)
		{	
//			nodeTmp.deltaScore = 0;
			g_pPath->InsertPath(g_pPath->flag2, nodeTmp, 0);
//path++;

		}
	}
/*
switch(SeedNode.flag)
{
	case 0:
		smallpath++;
		break;
	case 1:
		highpath[0]++;
		break;
	case 2:
		highpath[1]++;
		break;
	default:
		break;
}
if(!path)
{
	switch(SeedNode.flag)
	{
	case 0:
		small++;
		levelnum[0][SeedNode.probLevel]++;
		break;
	case 1:
		high[0]++;		
		levelnum[1][SeedNode.probLevel]++;
		break;
	case 2:
		high[1]++;		
		levelnum[2][SeedNode.probLevel]++;
		break;
	default:
		printf("flag error\n");
		getch();
		break;
	}
	//FILE*fp=fopen("path.txt","a+t");
	//fprintf(fp,"%-6d, %d\n",key,SeedNode.probLevel);
	//fclose(fp);
	
}
else
{
	switch(SeedNode.flag)
	{
	case 0:
		smalllevel[0][SeedNode.probLevel]++;
		break;
	case 1:
		smalllevel[1][SeedNode.probLevel]++;
		break;
	case 2:
		smalllevel[2][SeedNode.probLevel]++;
		break;
	default:
		printf("flag error\n");
		getch();
		break;
	}
}

*/
}

//以下由高声编辑，装载快速高斯计算的数据库
BOOL CAM::LoadShortList()
{
	int	i, j, len;
	FILE* fp;

	//获取文件路径
	TCHAR lpszFileName[MAX_PATH];
	DWORD dwSize = GetPrivateProfileString(amTopic, "ShortListIdx", NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	ASSERT(dwSize != 0);

	fp = fopen(lpszFileName, "rb");
	if(fp == NULL)
		return FALSE;
	fseek(fp, 0, SEEK_END);
	len = ftell(fp) / 6;
	m_pShortListNum = new short[len];
	m_pShortListOffset = new int[len];
	fseek(fp, 0, SEEK_SET);
	for(i = 0; i < len; i++)
	{
		fread(m_pShortListNum+i, sizeof(short), 1, fp);
		fread(m_pShortListOffset+i, sizeof(int), 1, fp);
	}
	fclose(fp);
	//
	dwSize = GetPrivateProfileString(amTopic, "ShortListDat", NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	fp = fopen(lpszFileName, "rb");
	if(fp == NULL)
		return 1;
	fseek(fp, 0, SEEK_END);
	len = ftell(fp) / sizeof(char);
	m_pShortList = new char[len];
	fseek(fp, 0, SEEK_SET);
	fread(m_pShortList, sizeof(char), len, fp);
	fclose(fp);
	//
	dwSize = GetPrivateProfileString(amTopic, "CodeBook", NULL,
		lpszFileName, MAX_PATH, "SearchEngine.ini");
	fp = fopen(lpszFileName, "rb");
	if(fp == NULL)
		return FALSE;
	m_pClusterMean = new float[CODE_BOOK_SIZE*NO_DIM];
	m_pClusterVar = new float[CODE_BOOK_SIZE*NO_DIM];
	for(i = 0; i < CODE_BOOK_SIZE; i++)
	{
		fread(m_pClusterMean+i*NO_DIM, sizeof(float), NO_DIM, fp);	
		fread(m_pClusterVar+i*NO_DIM, sizeof(float), NO_DIM, fp);
	}
	fclose(fp);
	return TRUE;
}

short CAM::FindCodebook(float *pVec)
{
	int		i, minIdx, l;
	float	diffTemp, ftmp, dis, minDis;

	minIdx = -1;
	minDis = POS_HUGE;
	for(i = 0; i < CODE_BOOK_SIZE; i++)
	{
		dis = 0;
		for(l = 0; l < NO_DIM; l++)  // Distance Calculation Option
		{
			diffTemp = pVec[l] - m_pClusterMean[i*NO_DIM+l];
			ftmp = diffTemp * diffTemp;
		//	dis += ftmp * clusterVar[i*No_Dim+l];
			dis += ftmp;
		}
		if(dis < minDis)
		{
			minDis = dis;
			minIdx = i;
		}
	}
	ASSERT(minIdx >= 0);
	return minIdx;
}

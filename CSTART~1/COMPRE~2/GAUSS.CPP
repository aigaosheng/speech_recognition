// Gauss.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "const.h"

const double logbase = 10*log(1.000005);
const double PAI2 = 8 * atan(1.0);		//2 * Pai

LPOUTPUT pOutput = NULL;
LPOUTPUT_ALIGNED pOutput_Aligned = NULL;

int LoadOutput(LPCTSTR lpszFileName)
{
	//计算扇区的大小
	DWORD dwSecPerCluster, dwBytePerSector, dwNumFreeCluster, dwNumTotalCluster;
	if (!GetDiskFreeSpace(NULL, &dwSecPerCluster, &dwBytePerSector, &dwNumFreeCluster, 
		&dwNumTotalCluster)){
		TRACE1("There is error when GetDiskFreeSpace(), Error num = %d\n", GetLastError());
		return 0;
	}

	//打开文件并获取文件大小
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, 
							  NULL, OPEN_EXISTING,
							  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, 
							  NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		return 0;
	}

	DWORD dwSize = GetFileSize(hFile, NULL);
	ASSERT(dwSize != -1);
	//ASSERT(dwSize == sizeof(OUTPUT) * NO_OUTPUT);
	if (dwSize % sizeof(OUTPUT) != 0){
		printf("Error in the output file!\n");
		CloseHandle(hFile);
		return 0;
	}

	int nOutput = dwSize / sizeof(OUTPUT);

	//分配内存
	pOutput = (OUTPUT *)VirtualAlloc(NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (pOutput == NULL){
		TRACE("There is not enough memory!\n");
		CloseHandle(hFile);
		return 0;
	}

	//计算文件所占扇区个数
	int	nBuffer = dwSize / dwBytePerSector;
	if (dwSize % dwBytePerSector)
		nBuffer++;

	//读取到缓冲区 
	DWORD dwNumByteRead;
	if (!ReadFile(hFile, (LPVOID)pOutput, dwBytePerSector * nBuffer, &dwNumByteRead, NULL)){
		TRACE1("Error when read Bigram Index file, Error Num = %d\n", GetLastError());
		CloseHandle(hFile);
		VirtualFree((LPVOID)pOutput, 0, MEM_RELEASE);
		return 0;
	}

	//关闭文件
	CloseHandle(hFile);

	return nOutput;
}

BOOL SaveOutput(LPCTSTR lpszFileName, int nOutput)
{
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_WRITE, 0, NULL,
					   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD dwNumByteOfWrite;
	WriteFile(hFile, (LPVOID)pOutput_Aligned, sizeof(OUTPUT_ALIGNED) * nOutput, &dwNumByteOfWrite, NULL);
	CloseHandle(hFile);

	return TRUE;
}
int main(int argc, char* argv[])
{
	if (argc != 3)
		printf("please use like this:\n\t gauss outputlist.dat NewOutputllist.dat\n");

	int numOutput;
	numOutput = LoadOutput(argv[1]);
	if (numOutput == 0)
		return 0;

	int nOutput, nMix, nDim;
	for (nOutput = 0; nOutput < numOutput; nOutput++){
		LPOUTPUT pCurOutput = pOutput + nOutput;

		for (nMix = 0; nMix < NO_MIX; nMix++){

			double CoeMix = pCurOutput->CoeMix[nMix];
			double Fenmu = PAI2;
			for (nDim = 0; nDim < NO_DIM; nDim++){
				Fenmu *= pCurOutput->Variance[nMix * NO_DIM + nDim];
			}
			pCurOutput->CoeMix[nMix] = CoeMix / sqrt(Fenmu);

			for (nDim = 0; nDim < NO_DIM; nDim++){
				pCurOutput->Variance[nMix * NO_DIM + nDim] 
					= 1.0 / (pCurOutput->Variance[nMix * NO_DIM + nDim] * (-2.0));
			}
		}
	}

	pOutput_Aligned = new OUTPUT_ALIGNED[numOutput];
	memset((void*)pOutput_Aligned, 0, sizeof(OUTPUT_ALIGNED) * numOutput);

	for (nOutput = 0; nOutput < numOutput; nOutput++){
		LPOUTPUT pCurOutput = pOutput + nOutput;

		for (nMix = 0; nMix < NO_MIX; nMix++){

			pOutput_Aligned[nOutput].CoeMix[nMix] = pCurOutput->CoeMix[nMix];

			for (nDim = 0; nDim < NO_DIM; nDim++){
				pOutput_Aligned[nOutput].Means[nMix][nDim] = pCurOutput->Means[nMix * NO_DIM + nDim];
				pOutput_Aligned[nOutput].Variance[nMix][nDim] = pCurOutput->Variance[nMix * NO_DIM + nDim];
			}
		}
	}
	
	if (!SaveOutput(argv[2], numOutput)){
		printf("Error when Save new outputlist!\n");
	}

	delete []pOutput_Aligned;
	VirtualFree(pOutput, 0, MEM_RELEASE);

	return 0;
}


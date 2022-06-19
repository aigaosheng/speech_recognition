// CompressMapping.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int main(int argc, char* argv[])
{
	printf("Compress the mapping file from int to Byte");
	if (argc != 3){
		printf("Please use like that:\n\tCompress???.exe OldFile NewFile");
		return 1;
	}

	HANDLE hFile = CreateFile(argv[1], GENERIC_READ, 0, NULL,
							  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		printf("Cann't open File %s", argv[1]);
		return 1;
	}

	DWORD dwSize = GetFileSize(hFile, NULL);
	if (dwSize % 4){
		printf("the size of the file is wrong");
		return FALSE;
	}

	int *pOld = new int[dwSize / 4];
	DWORD dwNumOfByteRead;
	BOOL bRet = ReadFile(hFile, pOld, dwSize, &dwNumOfByteRead, NULL);
	CloseHandle(hFile);

	BYTE *pNew = new BYTE[dwSize / 4];
	memset(pNew, 0, dwSize / 4);

	WORD pIndex[62];
	pIndex[0] = 0;
	int nIndex = 1;
	int temp = pOld[0];
	for (int i = 0; i < dwSize /4; i++){
		pNew[i] = pOld[i];
		if (pOld[i] > temp){
			pIndex[nIndex++] = i;
			temp = pOld[i];
		}
	}

	hFile = CreateFile(argv[2], GENERIC_WRITE, 0, NULL,
					   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD dwNumByteOfWrite;
	bRet = WriteFile(hFile, pNew, dwSize / 4, &dwNumByteOfWrite, NULL);
	CloseHandle(hFile);

	delete []pNew;
	delete []pOld;
	return 0;
}


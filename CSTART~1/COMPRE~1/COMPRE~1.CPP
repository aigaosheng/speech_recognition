// CompressTriphoneList.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "const.h"

typedef struct tag_TriphoneList
{
	WORD dwKey;
	WORD wClassTriphone;
} TRIPHONELIST;

TRIPHONELIST TriphoneList[1814 * 1024];

int triphoneMap[TOTAL_PHONES][5][6][6][TOTAL_PHONES][TOTAL_PHONES];
//DWORD TriphoneIndex[TOTAL_PHONES][TOTAL_PHONES];
WORD Initial[MAX_CODE_INITIAL_PART + 1][TOTAL_PHONES - MAX_CODE_INITIAL_PART - 1]
			[TOTAL_PHONES - MAX_CODE_INITIAL_PART - 2];
WORD Final[TOTAL_PHONES - MAX_CODE_INITIAL_PART - 2][MAX_CODE_INITIAL_PART + 1][MAX_CODE_INITIAL_PART + 2]
			[5][6][6];

int main(int argc, char* argv[])
{
	if (argc != 3){
		printf("Please use like that:\n\tCompress???.exe OldFile NewFile");
		return 1;
	}

	memset((void*)triphoneMap, 0xFF, sizeof(int) * 62*62*62*5*6*6);
	memset((void*)Initial, 0xFF, sizeof(WORD) * 24 * 38 * 37);
	memset((void*)Final, 0xFF, sizeof(WORD) * 37 * 24 * 25 * 6 * 6 * 5);

	//按照原来的方式打开原来的文件
	FILE *f1 = fopen(argv[1], "rb");
	//f1 = fopen("data\\triphoneList.dat", "rb");
	if(f1 == NULL)
	{
		printf("triphoneList.dat not found\n");
		exit(-1);
	}
	for(int i = 0; i <= MAX_CODE_INITIAL_PART; i++)
		fread(triphoneMap[i][0][0][0], sizeof(int), TOTAL_PHONES*TOTAL_PHONES, f1);	
	//for(i = MAX_CODE_INITIAL_PART+1; i <= MAX_CODE_FINAL_PART; i++)
	//	fread(triphoneMap[i], sizeof(int), 5*6*6*TOTAL_PHONES*TOTAL_PHONES, f1);	
	fread(triphoneMap+MAX_CODE_INITIAL_PART+1, sizeof(int), 
		5*6*6*TOTAL_PHONES*TOTAL_PHONES*(MAX_CODE_FINAL_PART-MAX_CODE_INITIAL_PART), f1);	
	fread(triphoneMap[SILENCE_CODE][0][0][0], sizeof(int), TOTAL_PHONES*TOTAL_PHONES, f1);
	fclose(f1);

	//重新组织文件
/*	DWORD nNum_Exist = 0;
	WORD wKey;
	WORD wTemp;
	for (WORD nBasePhone = 0; nBasePhone < TOTAL_PHONES; nBasePhone++){
		for (WORD nLeftPhone = 0; nLeftPhone < TOTAL_PHONES; nLeftPhone++){
			for (WORD nRightPhone = 0; nRightPhone < TOTAL_PHONES; nRightPhone++){
				wKey = nRightPhone << 9;

				for (WORD nBaseTone = 0; nBaseTone < 5; nBaseTone++){
					wKey = wKey & 0xFE00;				// 11111110 00000000
					wTemp = nBaseTone << 6;
					wKey = wKey | wTemp;

					for (WORD nLeftTone = 0; nLeftTone < 6; nLeftTone++){
						wKey = wKey & 0xFFC0;			// 11111111 11000000
						wTemp = nLeftTone << 3;
						wKey = wKey | wTemp;

						for (WORD nRightTone = 0; nRightTone < 6; nRightTone++){
							wKey = wKey & 0xFFF8;		// 11111111 11111000
							wKey = wKey | nRightTone;

							if (triphoneMap[nBasePhone][nBaseTone][nLeftTone]
								[nRightTone][nLeftPhone][nRightPhone] != -1){
								TriphoneList[nNum_Exist].dwKey = wKey;
								TriphoneList[nNum_Exist].wClassTriphone = (WORD)
									triphoneMap[nBasePhone][nBaseTone][nLeftTone]
									[nRightTone][nLeftPhone][nRightPhone];
								nNum_Exist++;
							}
						}
					}
				}
			}

			TriphoneIndex[nBasePhone][nLeftPhone] = nNum_Exist;
		}
	}
*/

	for (int nBasePhone = 0; nBasePhone < 24; nBasePhone++){
		for (int nLeftPhone = 0; nLeftPhone < 38; nLeftPhone ++)
			for (int nRightPhone = 0; nRightPhone < 37; nRightPhone++)
				Initial[nBasePhone][nLeftPhone][nRightPhone] = triphoneMap[nBasePhone][0][0][0]
									[nLeftPhone + 24][nRightPhone + 24];
	}
	for (nBasePhone = 0; nBasePhone < 37; nBasePhone++){
		for (int nLeftPhone = 0; nLeftPhone < 24; nLeftPhone++)
			for (int nRightPhone = 0; nRightPhone < 25; nRightPhone++){
				int nTempRightPhone = nRightPhone;
				if (nRightPhone == 24)
					nTempRightPhone = 61;
				for (int nBaseTone = 0; nBaseTone < 5; nBaseTone++)
					for (int nLeftTone = 0; nLeftTone < 6; nLeftTone++)
						for(int nRightTone = 0; nRightTone < 6; nRightTone++)
							Final[nBasePhone][nLeftPhone][nRightPhone][nBaseTone][nLeftTone][nRightTone]
							= triphoneMap[nBasePhone + 24][nBaseTone][nLeftTone][nRightTone][nLeftPhone][nTempRightPhone];
			}
	}

//	printf("Tatal number is %d\n", nNum_Exist);

	HANDLE hFileNew = CreateFile(argv[2], GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFileNew == INVALID_HANDLE_VALUE){
		printf("cann't create the %s File", argv[2]);
	}

	DWORD dwNumByteWritten;
	WriteFile(hFileNew, (LPVOID)Initial, sizeof(WORD) * 24 * 38 * 37, 
				&dwNumByteWritten, NULL);
	WriteFile(hFileNew, (void *)Final, sizeof(WORD) * 37 * 24 * 25 * 6 * 6 * 5, &dwNumByteWritten, NULL);

//	printf("sizeof(TRIPHONELIST) = %d, \nsizeof(TRIPHONELIST) * nNum_Exist = %d, \ndwNumByteWritten = %d\n",
//		sizeof(TRIPHONELIST), sizeof(TRIPHONELIST) * nNum_Exist, dwNumByteWritten);
	CloseHandle(hFileNew);
	return 0;
}


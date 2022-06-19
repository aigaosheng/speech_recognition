//PitchExtractor.h
//Header defination of PitchExtractor, DLL file
//Include  this header to export some functions and import funtions
//writtend and noted bye  zhangjian. 2000.3.31.
#include "afx.h"

#ifdef PITCHEXTRACTOR_EXPORTS
#define PITCHEXTRACTOR_API __declspec(dllexport)
#else
#define PITCHEXTRACTOR_API __declspec(dllimport)
#endif

PITCHEXTRACTOR_API BOOLEAN  InitPitchExtractor(void);
PITCHEXTRACTOR_API void     SetPitchExchangeBuf(short *pSampleDataBuf,int nSampleCountByShort,float *pPitchBuf);
PITCHEXTRACTOR_API BOOLEAN  ExtractPitch(void);

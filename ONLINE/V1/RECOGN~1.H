
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the RECOGNIZE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// RECOGNIZE_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef RECOGNIZE_EXPORTS
#define RECOGNIZE_API __declspec(dllexport)
#else
#define RECOGNIZE_API __declspec(dllimport)
#endif

// This class is exported from the Recognize.dll

//RECOGNIZE_API int fnRecognize(void);

int SrStartDragonVoice(char* modelType);

int SrMicOn(void);

int SrMicOff(void);

int SrExitDragonVoice(void);

void SrGetFirmWords(char *firmRes);
int SrLoadDictationMode(void);

//void SrSetMsgTarget(HWND  hwnd);

int SrPauseAudio(void);

int SrResumeAudio(void);

int SrGetAudioState(void);

int SrOfflineCepTest(char* dataFile);		// The data file is the cepstrum based

void SrGetPartialFirmWords(char*);
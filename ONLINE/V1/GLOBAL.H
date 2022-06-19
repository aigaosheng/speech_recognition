// Global.h: interface for the CGlobal class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GLOBAL_H__73B9C7D6_0511_460B_9BA6_94F46A77C852__INCLUDED_)
#define AFX_GLOBAL_H__73B9C7D6_0511_460B_9BA6_94F46A77C852__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AM.h"
#include "LM.h"

#include "Path.h"

#include "Lextree.h"
#include "WordLattice.h"

#include "Audio.h"

extern CAM				*g_pAM;
extern CLM				*g_pLM;
extern CPath			*g_pPath;
extern CLextree			*g_pLextree;
extern CWordLattice		*g_pWordLattice;

extern HANDLE g_hEvent;
extern BOOL g_fTerminate;
extern HANDLE g_RecogThread;
extern HWND g_hMsgTarget;

extern char *g_stHanziString[MAX_MODE_NUM];

extern CAudioManage		*g_pAudio;

extern DWORD	m_nHisWord1, m_nHisWord2; //¸ßÉýÌí¼Ó
extern long		eventCounter;


extern CModeManager *g_pModeManager;
extern void GetPartialFirmwords(void);
extern void GetLastFirmwords(void);

#endif // !defined(AFX_GLOBAL_H__73B9C7D6_0511_460B_9BA6_94F46A77C852__INCLUDED_)

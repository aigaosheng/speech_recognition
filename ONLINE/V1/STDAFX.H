// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__79CFFE3A_A388_4B07_B27D_51E8A77A69E3__INCLUDED_)
#define AFX_STDAFX_H__79CFFE3A_A388_4B07_B27D_51E8A77A69E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// 关闭部分无用的编译警告
// 保证使用 "warning level 4" 编译选项和优化编译选项时, 不出警告信息

// nonstandard extension used : nameless struct/union
#pragma warning(disable: 4201)

// unreferenced local function has been removed
#pragma warning(disable: 4514)

// unreferenced formal parameter
#pragma warning(disable: 4100)

// function xxxx selected for automatic inline expansion
#pragma warning(disable: 4711)

//identifier was truncated to '255' characters in the debug information
#pragma warning(disable: 4786)

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <search.h>
#include <windows.h>

// TODO: reference additional headers your program requires here
#define	USE_SSE
#ifdef USE_SSE
#include <xmmintrin.h>
#endif

#include "debug.h"
#include "PitchExtractor.h"

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__79CFFE3A_A388_4B07_B27D_51E8A77A69E3__INCLUDED_)


/////////////////////////////////////////////////////////////////////////////
//
// debug.cpp : 调试宏引用的内部调试函数的实现
//
// Win32 version
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef _DEBUG

// 向调试终端(或调试器)输出格式化的调试信息
void _DebugTrace(const char* pszFormat, ...)
{
    char szBuffer[1024];
    va_list args;

    va_start(args, pszFormat);
    wvsprintf(szBuffer, pszFormat, args);
    va_end(args);

    OutputDebugString(szBuffer);
}

// 断言失败处理
void WINAPI _AssertFailed(const char* pszFileName, int nLine)
{
    // 输出断言失败的调试信息
    TRACE2("Assertion failed: Line %d, File %s\n", nLine, pszFileName);
    
    // 进入调试器
    __asm int 3;
}

#endif  // _DEBUG
/////////////////////////////////////////////////////////////////////////////

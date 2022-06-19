
/////////////////////////////////////////////////////////////////////////////
//
// debug.h : 调试宏接口
//
// Win32 version
//
// Last modified date: 98-12-8
//
/////////////////////////////////////////////////////////////////////////////

/****************************************************************************

本接口定义了以下宏:

    . 用于断言的宏

        ASSERT
        VERIFY

    . 用于输出调试信息的宏

        TRACE
        TRACE0
        TRACE1
        TRACE2
        TRACE3
        TRACE4
        TRACE5
        TRACE6
        TRACE7
        TRACE8
        TRACE9

在使用之前, 先必须按以下方法定义 THIS_FILE 宏:

#include "debug.h"

#ifdef _DEBUG
#undef THIS_FILE
DEBUG_TEXT THIS_FILE[] = __FILE__;
#endif

****************************************************************************/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef _DEBUG

    // 内部调试函数
    void _DebugTrace(const char* pszFormat, ...);
    void WINAPI _AssertFailed(const char* pszFileName, int nLine);
    
    #define DEBUG_TEXT      static const char
    
    #define THIS_FILE       __FILE__
    
    #define ASSERT(exp)                                     \
            do                                              \
            {                                               \
                if (!(exp))                                 \
                    _AssertFailed(THIS_FILE, __LINE__);     \
            } while (0)

    #define VERIFY(exp)     ASSERT(exp)
    
    #define TRACE(sz)                                       \
            do                                              \
            {                                               \
                DEBUG_TEXT _sz[] = sz;                      \
                _DebugTrace(_sz);                           \
            } while (0)

    #define TRACE0          TRACE

    #define TRACE1(sz, p1)                                  \
            do                                              \
            {                                               \
                DEBUG_TEXT _sz[] = sz;                      \
                _DebugTrace(_sz, p1);                       \
            } while (0)

    #define TRACE2(sz, p1, p2)                              \
            do                                              \
            {                                               \
                DEBUG_TEXT _sz[] = sz;                      \
                _DebugTrace(_sz, p1, p2);                   \
            } while (0)

    #define TRACE3(sz, p1, p2, p3)                          \
            do                                              \
            {                                               \
                DEBUG_TEXT _sz[] = sz;                      \
                _DebugTrace(_sz, p1, p2, p3);               \
            } while (0)

    #define TRACE4(sz, p1, p2, p3, p4)                      \
            do                                              \
            {                                               \
                DEBUG_TEXT _sz[] = sz;                      \
                _DebugTrace(_sz, p1, p2, p3, p4);           \
            } while (0)
    
    #define TRACE5(sz, p1, p2, p3, p4, p5)                  \
            do                                              \
            {                                               \
                DEBUG_TEXT _sz[] = sz;                      \
                _DebugTrace(_sz, p1, p2, p3, p4, p5);       \
            } while (0)
    
    #define TRACE6(sz, p1, p2, p3, p4, p5, p6)              \
            do                                              \
            {                                               \
                DEBUG_TEXT _sz[] = sz;                      \
                _DebugTrace(_sz, p1, p2, p3, p4, p5, p6);   \
            } while (0)

    #define TRACE7(sz, p1, p2, p3, p4, p5, p6, p7)          \
            do                                              \
            {                                               \
                DEBUG_TEXT _sz[] = sz;                      \
                _DebugTrace(_sz, p1, p2, p3, p4, p5, p6,    \
                    p7);                                    \
            } while (0)
    
    #define TRACE8(sz, p1, p2, p3, p4, p5, p6, p7, p8)      \
            do                                              \
            {                                               \
                DEBUG_TEXT _sz[] = sz;                      \
                _DebugTrace(_sz, p1, p2, p3, p4, p5, p6,    \
                    p7, p8);                                \
            } while (0)
    
    #define TRACE9(sz, p1, p2, p3, p4, p5, p6, p7, p8, p9)  \
            do                                              \
            {                                               \
                DEBUG_TEXT _sz[] = sz;                      \
                _DebugTrace(_sz, p1, p2, p3, p4, p5, p6,    \
                    p7, p8, p9);                            \
            } while (0)
    
    // 关闭使用调试宏导致的编译警告信息

    // conditional expression is constant
    #pragma warning(disable: 4127)
    
#else

    inline void _DebugTrace(const char*, ...)
    {
    }
    
    #define ASSERT(exp)     ((void)0)
    #define VERIFY(exp)     ((void)(exp))

    #define TRACE           1 ? (void)0 : _DebugTrace
    #define TRACE0          1 ? (void)0 : _DebugTrace
    #define TRACE1          1 ? (void)0 : _DebugTrace
    #define TRACE2          1 ? (void)0 : _DebugTrace
    #define TRACE3          1 ? (void)0 : _DebugTrace
    #define TRACE4          1 ? (void)0 : _DebugTrace
    #define TRACE5          1 ? (void)0 : _DebugTrace
    #define TRACE6          1 ? (void)0 : _DebugTrace
    #define TRACE7          1 ? (void)0 : _DebugTrace
    #define TRACE8          1 ? (void)0 : _DebugTrace
    #define TRACE9          1 ? (void)0 : _DebugTrace
    
    // 关闭使用调试宏导致的编译警告信息
    
    // unreferenced local function has been removed
    #pragma warning(disable: 4505)
    
    // statement has no effect
    #pragma warning(disable: 4705)

#endif  // _DEBUG

#endif  // _DEBUG_H_
/////////////////////////////////////////////////////////////////////////////

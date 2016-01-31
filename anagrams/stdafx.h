// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef WIN32
#  include "targetver.h"
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <tchar.h>
#else
typedef char CHAR;
typedef wchar_t WCHAR;
typedef wchar_t _TCHAR;
typedef const wchar_t* LPCWSTR;
#define MAX_PATH 4096
#endif
#include <stdio.h>



// TODO: reference additional headers your program requires here

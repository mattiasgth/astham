#include "stdafx.h"
#include "application.h"
#include <algorithm>
#include <iostream>

extern Application* gpApp;

Application::Application()
{
}

Application::~Application()
{
}

//	wchar_t * filename = getCmdOption(argv, argv + argc, "-f");
//
//	if (filename)
//	{
//		do_interesting_things();
//	
//	}
wchar_t* Application::GetCmdOption(wchar_t ** begin, wchar_t ** end, const std::wstring & option)
{
	wchar_t ** itr = std::find(begin, end, option);
	if (itr != end && ++itr != end){
		return *itr;
	}
	return 0;
}

//
//  if(CmdOptionExists(argv, argv+argc, "-h"))
//	{
//		do_stuff();
//	}
//
bool Application::CmdOptionExists(wchar_t** begin, wchar_t** end, const std::wstring& option)
{
	return std::find(begin, end, option) != end;
}

int _tmain(int argc, _TCHAR* argv[])
{
	return gpApp->Run(argc, argv);
}

void ReportError(char* pszMsg)
{
	std::cerr << "Error" << std::endl << pszMsg << std::endl;
}

#ifdef _DEBUG
void ApplicationTrace(LPCTSTR lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);

	int nBuf;
	wchar_t szBuffer[512];

	nBuf = _vsnwprintf_s(szBuffer, _countof(szBuffer), _countof(szBuffer), lpszFormat, args);

	// was there an error? was the expanded string too long?
	_ASSERT(nBuf >= 0);

	OutputDebugStringW(szBuffer);

	va_end(args);
}
#endif


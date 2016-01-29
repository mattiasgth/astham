#pragma once
#include <string>

class Application
{
public:
	Application();
	virtual ~Application();
	virtual int Run(int argc, wchar_t* argv[]){ return 0; };
	wchar_t* GetCmdOption(wchar_t ** begin, wchar_t ** end, const std::wstring & option);
	bool CmdOptionExists(wchar_t** begin, wchar_t** end, const std::wstring& option);
};

void ReportError(char* pszMsg);

/*
*
*	Standard debugging helpers
*
*/
#ifndef NDEBUG
void ApplicationTrace(LPCTSTR, ...);
#	define TRACE		::ApplicationTrace
#else
inline void ApplicationTrace(LPCTSTR, ...) { }
#	define TRACE		1 ? (void)0 : ::ApplicationTrace
#endif


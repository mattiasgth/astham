#include "stdafx.h"
#include "application.h"
#include <string.h>
#include <algorithm>
#include <iostream>
#include <string>

#if 0
// requires full C++11 support, codecvt is in that standard, but g++ doesn't support it, yet
#include <codecvt>
template <typename T>
std::string toUTF8(const std::basic_string<T, std::char_traits<T>, std::allocator<T>>& source)
{
	std::string result;

	std::wstring_convert<std::codecvt_utf8_utf16<T>, T> convertor;
		    result = convertor.to_bytes(source);

		        return result;
}

template <typename T>
void fromUTF8(const std::string& source, std::basic_string<T, std::char_traits<T>, std::allocator<T>>& result)
{
	std::wstring_convert<std::codecvt_utf8_utf16<T>, T> convertor;
	        result = convertor.from_bytes(source);
}:
#endif


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

#ifdef WIN32
int _tmain(int argc, _TCHAR* argv[])
{
	return gpApp->Run(argc, argv);
}
#else
int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "sv_SE.UTF8");
	wchar_t** wargv = new wchar_t*[argc];
	for(int i = 0; i < argc; i++){
		wargv[i] = new wchar_t[(strlen(argv[i]) + 1)*sizeof(wchar_t)];
		mbstowcs(wargv[i], argv[i], strlen(argv[i]));
	}
	// std::wcerr << argv[2] << std::endl;
	int rslt = gpApp->Run(argc, wargv);
	for(int i = 0; i < argc; i++){
		delete wargv[i];
	}
	delete wargv;
}
#endif

void ReportError(const char* pszMsg)
{
	std::cerr << "Error" << std::endl << pszMsg << std::endl;
}

#ifdef _DEBUG
void ApplicationTrace(const wchar_t* lpszFormat, ...)
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


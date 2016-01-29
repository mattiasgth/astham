// anagrams.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "application.h"
#include "dictionary.h"
#include <fzindex.h>
#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <locale>
#include <io.h>
#include <fcntl.h>

// naive but portable
std::string naiveWideToChar(std::wstring& input){
	std::string rslt;
	for (std::wstring::iterator it = input.begin(); it != input.end(); it++){
		int c = (int)*it;
		if ((c > 127) || (c < 0))
			throw std::exception("character out of range for naive conversion");
		rslt += (char)c;
	}
	return rslt;
}
std::string naiveWideToChar(const WCHAR* input){
	return naiveWideToChar(std::wstring(input));
}

class AsthamApplication : public Application
{
public:
	AsthamApplication();
	~AsthamApplication();
	int Run(int argc, _TCHAR* argv[]);
protected:
	void PrintUsage();
	// generation of anagrams
	Dictionary* m_pDict;

	typedef enum
	{
		PGM_FIND_FIRST = 0,
		PGM_GENERATE_ONLY = 1,
		PGM_COMPLETE = 2
	}PGM_ACTION;
};

AsthamApplication theApp;
Application* gpApp = &theApp;

AsthamApplication::AsthamApplication()
: m_pDict(NULL)
{
}

AsthamApplication::~AsthamApplication()
{
	delete m_pDict;
}

void AsthamApplication::PrintUsage()
{
	std::cerr << "usage: anagrams  <words> (-i|<expr>|-c <file>) [-l <loc>] [-s <n>] [-f|-g] [-m <n>]" << std::endl << std::endl;
	std::cerr << "  <words> dictionary to use, text file or pre-compiled FZI dictionary" << std::endl;
	std::cerr << "  <expr>  expression to use for generating anagrams" << std::endl;
	std::cerr << "  -i      interactive mode, read expressions from standard input" << std::endl;
	std::cerr << "  -l      locale to use, <loc> defaults to 'German_germany'" << std::endl;
	std::cerr << "  -s      random seed number" << std::endl;
	std::cerr << "  -f      find first complete anagram of <expression>, and exit" << std::endl;
	std::cerr << "  -g      generate all possible words from the letters in <expression>" << std::endl;
	std::cerr << "  -m      limit number of results to <n>" << std::endl;
	std::cerr << "  -c      save compiled dictionary as <file>" << std::endl;
}

int AsthamApplication::Run(int argc, _TCHAR* argv[])
{
	// program arguments
	String strDictionaryFile;
	String strExpression;
	std::string strOutputFile;
	std::string strLocale("German_germany");
	bool fInteractive = false;
	bool fCompileDictionary = false;
	PGM_ACTION whatToDo = PGM_COMPLETE;
	int nSeed = (int)time(NULL);
	unsigned maxResults = 256;

	// we want to output UTF8 strings
	_setmode(_fileno(stdout), _O_U8TEXT);
	// and read UTF16 strings
	_setmode(_fileno(stdin), _O_WTEXT);

	if (argc < 2){
		PrintUsage();
		return 0;
	}



	for (int i = 1; i < argc; i++){
		_TCHAR* cp = argv[i];
		switch (*cp){
		default:
			if (strDictionaryFile.empty())
				strDictionaryFile = argv[i];
			else if (strExpression.empty())
				strExpression = argv[i];
			break;
		case '/':
		case '-':
			cp++;
			switch (*cp){
			case 'm':
				i++;
				if (!argv[i]){
					ReportError("fatal: missing -m output limit parameter");
					return -45;
				}
				maxResults = _wtoi(argv[i]);
				break;
			case 's':
				i++;
				if (!argv[i]){
					ReportError("fatal: missing -s random seed parameter");
					return -45;
				}
				nSeed = _wtoi(argv[i]);
				break;
			case 'f':
				whatToDo = PGM_FIND_FIRST;
				break;
			case 'g':
				whatToDo = PGM_GENERATE_ONLY;
				break;
			case 'l':
				i++;
				if (!argv[i]){
					ReportError("fatal: missing -l locale parameter");
					return -46;
				}
				try{
					strLocale = naiveWideToChar(argv[i]);
				}
				catch (std::exception& e){
					std::cerr << "error making string conversion of locale name: " << e.what() << std::endl;
					return -622;
				}
				break;
			case 'i':
				// interactive mode
				fInteractive = true;
				break;
			case 'c':
				i++;
				if (!argv[i]){
					ReportError("fatal: missing -c dictionary output parameter");
					return -46;
				}
				try{
					strOutputFile = naiveWideToChar(argv[i]);
				}
				catch (std::exception& e){
					std::cerr << "error making string conversion of file name: " << e.what() << std::endl;
					return -62;
				}
				fCompileDictionary = true;
				break;
			case '?':
				PrintUsage();
				return 0;
			default:
				std::wcerr << L"fatal: unknown command line parameter: " << cp << std::endl;
				PrintUsage();
				return -1;
			};
			break;
		};
	}
	std::locale* ploc = NULL;
	try{
		ploc = new std::locale(strLocale);
		std::locale::global(*ploc);
	}
	catch (std::runtime_error& e){
		std::cerr << "error: " << e.what() << std::endl;
		return -623;
	}

	if (fCompileDictionary){
		if (fInteractive){
			ReportError("fatal: cannot have both -c and -i parameters");
			return -2;
		}
		if (strOutputFile.empty()){
			ReportError("fatal: missing -c output parameters");
			return -4;
		}
	}

	// the expression argument is argv[2]
	for (String::iterator it = strExpression.begin(); it != strExpression.end(); it++){
		if (isalpha(*it, *ploc)){
			// TODO: there's a bunch of sloppy conversion attempts in the dictionary, remove them
			// and accept only lower case input there
			*it = std::tolower(*it, *ploc);
		}
	}

	if (strDictionaryFile.empty()){
		ReportError("fatal: need a dictionary to work with");
		return 0;
	}

	// read dictionary file
	StringList lst;
	StringList lstResult;
	m_pDict = new Dictionary(nSeed);
	switch (m_pDict->DetectFileType(strDictionaryFile.c_str())){
		default:
		case Dictionary::DFT_NONE:
			ReportError("fatal: cannot read that input dictionary");
			return -71;
		case Dictionary::DFT_TEXT:
			m_pDict->ReadTextFile(argv[1]);
			break;
		case Dictionary::DFT_BINARY:
			m_pDict->ReadBinaryFile(argv[1]);
			break;
	}
	if (fCompileDictionary){
		std::cerr << "Saving dictionary, ";
		m_pDict->SaveBinaryFile(strOutputFile);
		std::cerr << "OK, done." << std::endl;
		return 0;
	}
	AnagramResult rslt;
	try{
		for (;;){
			if (fInteractive){
				// wcin won't work as expected
				String s;
				while (s.empty()){
					if (!std::getline(std::wcin, s))
						break;
				}
				strExpression = s;
			}
			switch (whatToDo){
			case PGM_FIND_FIRST:
				m_pDict->FindFirstCompleteAnagram(strExpression, rslt);
				break;
			case PGM_GENERATE_ONLY:
				m_pDict->Generate(strExpression, rslt, true, maxResults);
				break;
			default:
			case PGM_COMPLETE:
				// find all words that we can construct from the letters in argv[1]
				m_pDict->Generate(strExpression, rslt, false, maxResults);
				break;
			}
			// print that
			unsigned i = 0;
			for (StringList::iterator it = rslt.begin(); it != rslt.end(); it++){
				if (i++ >= maxResults)
					break;
				std::wcout << (*it) << std::endl;
			}
			rslt.clear();
			if (!fInteractive)
				break;
		}
	}
	catch (...){
		ReportError("unknown exception caught, exiting");
		return -1;
	}
	// everything ok
	return 0;
}





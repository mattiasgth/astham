// anagrams.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "anagrams.h"
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
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif


// TODO: Report that file wasn't found when trying to read a non-existing one

// naive but portable
inline int safe_wtoi(const wchar_t *str)
{
  return (int)wcstol(str, 0, 10);
}

// naive but portable
std::string naiveWideToChar(std::wstring& input){
	std::string rslt;
	for (std::wstring::iterator it = input.begin(); it != input.end(); it++){
		int c = (int)*it;
		if ((c > 127) || (c < 0))
			throw std::logic_error("character out of range for naive conversion");
		rslt += (char)c;
	}
	return rslt;
}
std::string naiveWideToChar(const wchar_t* input){
	std::wstring s(input);
	return naiveWideToChar(s);
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
	std::cerr << "  -w      text input is encoded as UTF-16 (default UTF-8)" << std::endl;
	std::cerr << "  -v      verbose mode" << std::endl;
}

int AsthamApplication::Run(int argc, _TCHAR* argv[])
{
	// program arguments
	std::string strDictionaryFile;
	String strExpression;
	std::string strOutputFile;
	std::string strLocale("C");
	bool fInteractive = false;
	bool fCompileDictionary = false;
	bool fVerbose = false;
	bool fTextInputIsUTF16 = false;
	PGM_ACTION whatToDo = PGM_COMPLETE;
	int nSeed = (int)time(NULL);
	unsigned maxResults = 256;

#ifdef WIN32
	// we want to output UTF8 strings
	_setmode(_fileno(stdout), _O_U8TEXT);
	// and read UTF16 strings
	_setmode(_fileno(stdin), _O_WTEXT);
#endif

	if (argc < 2){
		PrintUsage();
		return ASTHAM_NOT_ENOUGH_PARAMS;
	}



	for (int i = 1; i < argc; i++){
		_TCHAR* cp = argv[i];
		switch (*cp){
		default:
			if (strDictionaryFile.empty())
				strDictionaryFile = naiveWideToChar(argv[i]);
			else if (strExpression.empty()){
				strExpression = argv[i];
			}
			break;
		case '/':
		case '-':
			cp++;
			switch (*cp){
			case 'v':
				fVerbose = true;
				break;
			case 'm':
				i++;
				if (!argv[i]){
					ReportError("fatal: missing -m output limit parameter");
					return ASTHAM_MISSING_PARAMETER;
				}
				maxResults = safe_wtoi(argv[i]);
				break;
			case 's':
				i++;
				if (!argv[i]){
					ReportError("fatal: missing -s random seed parameter");
					return ASTHAM_MISSING_PARAMETER;
				}
				nSeed = safe_wtoi(argv[i]);
				break;
			case 'f':
				whatToDo = PGM_FIND_FIRST;
				break;
			case 'w':
				fTextInputIsUTF16 = true;
				break;
			case 'g':
				whatToDo = PGM_GENERATE_ONLY;
				break;
			case 'l':
				i++;
				if (!argv[i]){
					ReportError("fatal: missing -l locale parameter");
					return ASTHAM_MISSING_LOCALE;
				}
				try{
					strLocale = naiveWideToChar(argv[i]);
				}
				catch (std::exception& e){
					std::cerr << "error making string conversion of locale name: " << e.what() << std::endl;
					return ASTHAM_BAD_LOCALE_STRING_CONVERSION;
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
					return ASTHAM_MISSING_PARAMETER;
				}
				try{
					strOutputFile = naiveWideToChar(argv[i]);
				}
				catch (std::exception& e){
					std::cerr << "error making string conversion of file name: " << e.what() << std::endl;
					return ASTHAM_BAD_FILE_STRING_CONVERSION;
				}
				fCompileDictionary = true;
				break;
			case '?':
				PrintUsage();
				return 0;
			default:
				std::wcerr << L"fatal: unknown command line parameter: " << cp << std::endl;
				PrintUsage();
				return ASTHAM_UNKNOWN_PARAMETER;
			};
			break;
		};
	}
	if(fVerbose){
		std::cerr << "expression is ";
		if(strExpression.empty())
			std::cerr << "<empty>";
		else
			std::wcerr << strExpression;
		std::cerr << std::endl;
	}
	std::locale* ploc = NULL;
	if(fVerbose){
		std::cerr << "setting locale " << strLocale << std::endl;
	}
	try{
		ploc = new std::locale(strLocale.c_str());
		std::locale::global(*ploc);
	}
	catch (std::runtime_error& e){
		std::cerr << "error: " << e.what() << std::endl;
		std::cerr << "when trying to create a locale named '" << strLocale << "'" << std::endl;
		return ASTHAM_CANNOT_CREATE_LOCALE;
	}

	if (fCompileDictionary){
		if (strOutputFile.empty()){
			ReportError("fatal: missing -c output parameters");
			return ASTHAM_MISSING_PARAMETER;
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
		return ASTHAM_NO_DICTIONARY_PARAM;
	}

	// read dictionary file
	StringList lst;
	StringList lstResult;
	m_pDict = new Dictionary(nSeed);
	m_pDict->SetVerbose(fVerbose);
	switch (m_pDict->DetectFileType(strDictionaryFile)){
		default:
		case Dictionary::DFT_NONE:
			ReportError("fatal: cannot read that input dictionary, DetectFileType returned DFT_NONE");
			return ASTHAM_CANNOT_READ_INPUT_DICTIONARY;
		case Dictionary::DFT_TEXT:
			if(fVerbose)
				std::cerr << "reading text file ..." << std::endl;
			m_pDict->ReadTextFile(strDictionaryFile, fTextInputIsUTF16);
			break;
		case Dictionary::DFT_BINARY:
			if(fVerbose)
				std::cerr << "reading binary file ..." << std::endl;
			m_pDict->ReadBinaryFile(strDictionaryFile);
			break;
	}
	if(fVerbose){
		std::cerr << "done, having " << m_pDict->GetNodeCount() << " nodes and " << m_pDict->GetStoredCount() << " stored items." << std::endl;
	}
	if (fCompileDictionary){
		std::cerr << "Saving dictionary, ";
		m_pDict->SaveBinaryFile(strOutputFile);
		std::cerr << "OK, done." << std::endl;
		return 0;
	}
	AnagramResult rslt;
	try{
		bool fKeepLooping = true;
		while (fKeepLooping){
			if (fInteractive){
				// wcin won't work as expected
				String s;
				while (s.empty()){
					if (!std::getline(std::wcin, s)){
						fKeepLooping = false;
						break;
					}
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
	catch (std::runtime_error& e){
		ReportError("caught runtime exception, exiting");
		return 0; // not an error as such
	}
	catch (...){
		ReportError("unknown exception caught, exiting");
		return ASTHAM_CAUGHT_EXCEPTION;
	}
	// everything ok
	return 0;
}





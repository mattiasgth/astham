#pragma once

#include <map>
#include <list>
#include <vector>
#include <sstream>
#include <fzindex.h>

typedef enum{
	DE_NONE,
	DE_NO_SUCH_FILE,
	DE_BAD_FORMAT,
	DE_BAD_VERSION,
	DE_FAILED_OPEN,
	DE_FAIL
}DICT_ERROR;

typedef bool(*DICT_GENERATE_CALLBACK)(int nPercent, unsigned long dwCookie);

typedef std::wstring String;

class StringList : public std::list<String>
{
public:
	bool ContainsString(const wchar_t* sz){
		iterator it;

		for (it = begin(); it != end(); it++){
			if (wcscmp((*it).c_str(), sz) == 0)
				return true;
		}
		return false;
	};
};

class StringArray : public std::vector<String>
{
};

class AnagramResult : public StringList
{
public:
	AnagramResult(){};
	bool IsEmpty()
	{
		return empty();
	}
	void AppendAnagram(String s)
	{
		push_back(s);
	}
	void AppendWord(String s)
	{
		for (iterator it = begin(); it != end(); it++){
			(*it).append(L" ");
			(*it).append(s);
		}
	}
	void MergeResult(AnagramResult& r)
	{
		for (StringList::iterator it = r.begin(); it != r.end(); it++){
			push_back(*it);
		}
	}
};

class IntArray : public std::vector<int>
{
};

//
//
//	StringX - class for sorting strings by quality
//
//
struct StringX : public String
{
public:
	StringX(String s) : inherited(s), m_nSpaces(0)
	{
		LPCWSTR pch;
		pch = c_str();
		while (*pch){
			if (iswspace(*pch++))
				m_nSpaces++;
		}
	};

	bool operator < (const StringX& op) const
	{
		return m_nSpaces < op.m_nSpaces;
	};

	typedef String inherited;

	int m_nSpaces;
};


class StringXList : public std::list<StringX>
{
};

///
/// CombMap
///
/// \brief Used for mapping string lengths and letters to the strings generated
class CombMap : public std::map<unsigned, std::vector<unsigned> >
{
};

#include "character_rack.h"

class AnagramSolutions : public std::map<String, AnagramResult>
{
public:
	void Store(size_t pos, CharacterRack& r, AnagramResult& a)
	{
		std::wstringstream strm;
		strm << pos << L";" << r;
		this->insert(std::make_pair(strm.str(), a));
	}
	bool FindSolution(size_t pos, CharacterRack& r, AnagramResult& out)
	{
		std::wstringstream strm;
		strm << pos << L";" << r;
		iterator it = find(strm.str());
		
		if (it != end()){
			out = at(strm.str());
			return true;
		}
		return false;
	}
};

class Dictionary
{
public:
	Dictionary(int nRandomSeed);
	~Dictionary();

	typedef enum{
		DFT_NONE,
		DFT_TEXT,
		DFT_BINARY
	}DICTIONARY_FILE_TYPE;
	DICTIONARY_FILE_TYPE DetectFileType(std::string& strPath);
	DICT_ERROR ReadTextFile(std::string& strPath, bool fIsUTF16);
	DICT_ERROR ReadBinaryFile(std::string& strPath);
	DICT_ERROR SaveBinaryFile(std::string& strPath);
	int GetNodeCount();
	int GetStoredCount();
	void SetVerbose(bool fset){ m_verbose = fset; };
protected:
	DICT_GENERATE_CALLBACK m_pfnCallback;
	unsigned long m_dwCookie;
	bool ParseTextLine(const wchar_t* pszLine);
	_tag_TRI_TREE* m_pTree;

	/// generation of words that can be made up from letters in input
public:
	int FindFirstCompleteAnagram(String& strInput, AnagramResult& rsltOut);
	int Generate(String& strInput, AnagramResult& rsltOut, bool fGenerateWordsOnly, unsigned maxResults = 256);
protected:
	int ReFindFirstCompleteAnagram(size_t pos, AnagramResult& lst_out, CharacterRack& rack);
	int ReGenerate(size_t pos, AnagramResult& lst_out, CharacterRack& rack, AnagramSolutions& s);
	static bool RackIsEmpty(LPCWSTR pszRack)
	{
		while (*pszRack){
			if (*pszRack++ != '#')
				return false;
		}

		return true;
	}

protected:
	/// combine words in a list so that as many as possible letters in a given set are utilized
	int Combine(CharacterRack& rack, AnagramResult& lst_partial, AnagramResult& lst_out);
	bool ReMakeCombines(StringArray& v, int idx, CombMap& m, CharacterRack& rack, AnagramResult& lst_out);
	int GetNode(size_t pos, TRI_FILE_NODE& fn);
	int FindStartNode(WCHAR c);
	// pseudo random stuff
	bool TrueOrFalse(){
		if (m_decisionpos++ > 32)
			m_decisionpos = 0;
		if ((m_decisionbits >> m_decisionpos) & 1)
			return true;
		return false;
	}
	std::uniform_int_distribution<int> m_dist;
	std::mt19937  m_gen;
	unsigned long m_decisionbits;
	int m_decisionpos;
	unsigned m_maxResults;
	bool m_verbose;
};


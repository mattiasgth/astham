#include "stdafx.h"
#include "application.h"
#include "anagrams.h"
#include "dictionary.h"
#include <fzindex.h>
#include <string.h>
#include <string>
#include <map>
#include <algorithm>
#include <iostream>

#ifndef __STDC_LIB_EXT1__

typedef int errno_t;

errno_t fopen_s(FILE** streamptr,
	const char* filename,
	const char* mode)
{
	*streamptr = fopen(filename, mode);
	return *streamptr ? 0 : errno;
}

int MulDiv(
  int nNumber,
  int nNumerator,
  int nDenominator
)
{
	return nNumber*nNumerator/nDenominator;
}

wchar_t *_wcsdup(
   const wchar_t *strSource 
)
{
	size_t s = wcslen(strSource);
	wchar_t* rslt = (wchar_t*)malloc(sizeof(wchar_t)*(s+1));
	wcscpy(rslt, strSource);
	return rslt;
}


#endif



Dictionary::Dictionary(int seed)
 :  m_gen(seed),
	m_pTree(NULL),
	m_pfnCallback(NULL),
	m_dwCookie(0),
	m_decisionpos(0),
	m_maxResults(256)
{
	m_decisionbits = m_dist(m_gen);
}

Dictionary::~Dictionary()
{
}

int Dictionary::GetNodeCount()
{
	return m_pTree->lNumNodes;
}

int Dictionary::GetStoredCount()
{
	return m_pTree->lNumStored;
}


Dictionary::DICTIONARY_FILE_TYPE Dictionary::DetectFileType(std::string& strPath)
{
	size_t n;
	FILE* fp;
	size_t rs;
	unsigned char szBuf[1024];
	if (TstCheckFileHeader(strPath.c_str()) == TST_ERROR_SUCCESS)
		return DFT_BINARY;

	fp = fopen(strPath.c_str(), "rb");

	if (fp == NULL){
		return DFT_NONE;
	}

	// ugly hack, but works for a lot of file types
	memset(szBuf, 0, sizeof(szBuf));
	rs = fread(szBuf, 1, 1024, fp);
	fclose(fp);
#ifdef WIN32
	int tst = IS_TEXT_UNICODE_UNICODE_MASK;
	if (IsTextUnicode(szBuf, rs, &tst)){
		// IsTextUnicode isn't the most
		// stable of the Win32 functions ...
		if (wcslen((wchar_t*)szBuf) < (rs / sizeof(wchar_t)))
			return DFT_NONE;
		return DFT_TEXT;
	}
#endif

	// some known BOM markers
	if(szBuf[0] == 0xff && szBuf[1] == 0xfe){
		return DFT_TEXT;
	}

	if(szBuf[1] == 0xff && szBuf[0] == 0xfe){
		return DFT_TEXT;
	}



	for (n = 0; n < rs; n++){
		if (szBuf[n] < 9){
			// probably UTF16
			char mbs[4096];
			if(wcstombs(mbs, (const wchar_t*)szBuf, 4096) == (size_t)-1){
				std::cerr << "not a UTF16 file" << std::endl;
				wprintf(L"Text is %s\n", szBuf);
				return DFT_NONE;
			}
			break;
		}
	}

	return DFT_TEXT;
}

DICT_ERROR Dictionary::ReadBinaryFile(std::string& strPath)
{
	long rslt;
	TstDestroyTree(m_pTree);
	m_pTree = TstInitTree();

	rslt = TstAttachToFile(m_pTree, strPath.c_str(), true);
	if (rslt == -2)
		return DE_BAD_VERSION;
	if (rslt == -1)
		return DE_FAILED_OPEN;
	return DE_NONE;
}

DICT_ERROR Dictionary::SaveBinaryFile(std::string& strPath)
{
	TstWriteToFile(m_pTree, strPath.c_str(), true);
	return DE_NONE;
}


DICT_ERROR Dictionary::ReadTextFile(std::string& strPath, bool fIsUTF16)
{
	unsigned char buf[2048];
	wchar_t* wp = (wchar_t*)buf;
	FILE* fp;

	fp = fopen(strPath.c_str(), "rb");

	if (fp == NULL)
		return DE_FAILED_OPEN;

	TstDestroyTree(m_pTree);
	m_pTree = TstInitTree();

	/* test for Unicode signatures */
	size_t r;
	int tst;
	unsigned lineno = 0;
	size_t start = 0;

	r = fread(buf, 1, 512, fp);
#ifdef WIN32
	if (!IsTextUnicode(wp, r, &tst)){
		/* text mode will do ANSI->Unicode conversion */
		fclose(fp);
		_tfopen_s(&fp, strPath.c_str(), _T("r"));
	}
	else{
		/* binary mode will do no conversion */
		rewind(fp);
		if (tst & IS_TEXT_UNICODE_SIGNATURE){
			start = 2;
		}
	}
#else
	if(buf[0] == 0xff && buf[1] == 0xfe)
		start = 2;
#endif
	fseek(fp, start, SEEK_SET);
	if(m_verbose)
		std::cerr << "reading from offset " << start << std::endl;
	while (!feof(fp)){
		if(fIsUTF16){
			lineno++;
			if (!fgetws(wp, 2048, fp)){
				std::cerr << "finished reading at line " << lineno << std::endl;
				break;
			}
		}
		else{
			char mbs[2048];
			size_t convres;
			lineno++;
			fgets(mbs, 2048, fp);
			convres = mbstowcs(wp, mbs, 2048);
			if(convres == (size_t)-1){
				std::cerr << "error reading line " << lineno << ", bad conversion" << std::endl;
				return DE_BAD_FORMAT;
			}
		}
		if (!ParseTextLine(wp)){
			std::cerr << "cannot parse line " << lineno << std::endl;
			fclose(fp);
			return DE_BAD_FORMAT;
		}
	}

	if(m_verbose){
		std::cerr << "read " << lineno << " number of lines" << std::endl;
	}
	fclose(fp);
	return DE_NONE;
}


bool Dictionary::ParseTextLine(const wchar_t* pszLine)
{
	bool reading_alpha = false;
	std::wstring s;
	const wchar_t* psz = pszLine;
	while(*psz){
		if(iswalpha(*psz)){
			reading_alpha = true;
			s.push_back(*psz);
		}
		else{
			if(reading_alpha){
				TstStoreString(m_pTree, (CPCHAR_T)s.c_str(), NULL, 0);
				s.clear();
			}
			reading_alpha = false;
		}
		psz++;
	}
	return true;

#ifndef WIN32
	std::cerr << "Error: ParseTextLine not implemented on this platform" << std::endl;
	return false;
#else
	// TODO: use isalpha or similar instead
	static LPCWSTR kszSeparators = L" .;,:!?\"#¤%&/()=\\+}{}[]$£@0123456789\t\r\n";
	// should do this with stl, but who knows
	// what to use?
	wchar_t* pszCopy;
	wchar_t* pszTok;
	wchar_t* pszContext;

	pszCopy = _wcsdup(pszLine);
	pszTok = wcschr(pszCopy, '#');
	if (pszTok)
		*pszTok = 0; /* cut string at comments */

	pszTok = wcstok_s(pszCopy, kszSeparators, &pszContext);
	do{
		if (pszTok && wcslen(pszTok) != 0){
			TstStoreString(m_pTree, (CPCHAR_T)pszTok, NULL, 0);
		}
		pszTok = wcstok_s(NULL, kszSeparators, &pszContext);
	} while (pszTok);
	free(pszCopy);
	return true;
#endif
}

int Dictionary::ReFindFirstCompleteAnagram(size_t pos, AnagramResult& lst_out, CharacterRack& rack)
{
	TRI_FILE_NODE fn;
	size_t rackPos;
	char c = GetNode(pos, fn);
	if (rack.IsEmpty()){		
		return 0; // no more letters to work with
	}
	if (!c)
		return 0;
	if (rack.Exists(c, rackPos)){
		rack.Cross(rackPos);
		if (ReFindFirstCompleteAnagram(fn.m_pos, lst_out, rack)){
			return 1;
		}
		if (fn.data_pos){
			if (rack.IsEmpty()){
				// problem solved!
				lst_out.AppendAnagram(rack.CurrentWord());
				return 1;
			}
			CharacterRack recRack(rack);
			CharacterRack uniRack(rack);
			uniRack.MakeRandomUnique(m_gen);
			for (CharacterRack::iterator it = uniRack.begin(); it != uniRack.end(); it++){
				if (ReFindFirstCompleteAnagram((size_t)m_pTree->root, lst_out, recRack)){
					lst_out.AppendWord(rack.CurrentWord());
					return 1;
				}
			}
		}
		rack.Uncross(c, rackPos);
	}
	int firstNode = 0;
	int secondNode = 0;
	if (TrueOrFalse()){
		firstNode = fn.l_pos;
		secondNode = fn.r_pos;
	}
	else{
		firstNode = fn.r_pos;
		secondNode = fn.l_pos;
	}
	if (ReFindFirstCompleteAnagram(firstNode, lst_out, rack))
		return 1;
	if (ReFindFirstCompleteAnagram(secondNode, lst_out, rack))
		return 1;
	return 0;
}

int Dictionary::FindFirstCompleteAnagram(String& strInput, AnagramResult& rsltOut)
{
	if (m_pTree == NULL){
		ReportError("no dictionary loaded");
		return 2;
	}
	CharacterRack rack(strInput);
	CharacterRack rackUnique(rack);
	rackUnique.MakeRandomUnique(m_gen);
	// for each unique letter on the rack
	//   for each word in the dictionary starting with that letter
	//		if we can find a complete anagram with the current rack, return it
	for (CharacterRack::iterator it = rackUnique.begin(); it != rackUnique.end(); it++){
		size_t pos = FindStartNode(*it);
		if (pos){
			if (ReFindFirstCompleteAnagram(pos, rsltOut, rack))
				return 1;
		}
	}
	return 0;
}

int Dictionary::Generate(String& strInput, AnagramResult& rsltOut, bool fGenerateWordsOnly, unsigned maxResults /* = 256*/)
{
	bool fBreak = false;
	StringList::iterator slit;

	if (m_pTree == NULL){
		ReportError("no dictionary loaded");
		return 2;
	}

	m_maxResults = maxResults;

	CharacterRack rack(strInput);
	AnagramSolutions s;
	AnagramResult rslt;
	if (fGenerateWordsOnly){
		ReGenerate((size_t)m_pTree->root, rsltOut, rack, s);
	}
	else{
		AnagramResult lstPartial;
		ReGenerate((size_t)m_pTree->root, lstPartial, rack, s);
		Combine(rack, lstPartial, rsltOut);
	}
	return rsltOut.empty() ? 0 : 1;
}

int Dictionary::FindStartNode(WCHAR cFind)
{
	WCHAR b1[2];
	WCHAR b2[2];
	b1[0] = cFind;
	b1[1] = '\0';
	b2[1] = '\0';
	TRI_FILE_NODE fn;
	// locate position of words starting with cFind
	size_t pos = (size_t)m_pTree->root;
	while (pos){
		WCHAR c = GetNode(pos, fn);
		if (!c){
			// next letter on rack
			return 0;
		}
		b2[0] = c;
		int cmp = wcscoll(b1, b2);
		if (cmp == 0){
			return pos;
		}
		if (cmp < 0)
			pos = fn.r_pos;
		else
			pos = fn.l_pos;
	}
	return 0;
}

int Dictionary::GetNode(size_t pos, TRI_FILE_NODE& fn)
{
	if (!pos) // nothing more to look for, this is a dead end
		return 0;
	if (m_pTree->flags & TSFLAG_FILE_BASED){
		if (!LoadFileNode(m_pTree, &fn, pos))
			return 0;
	}
	else{
		TRI_NODE* pNodeCurrent;
		// artificially create a file node
		pNodeCurrent = (TRI_NODE*)pos;
		fn.m_pos = (size_t)pNodeCurrent->m;
		fn.l_pos = (size_t)pNodeCurrent->l;
		fn.r_pos = (size_t)pNodeCurrent->r;
		fn.key = pNodeCurrent->key;
		fn.data_pos = (size_t)pNodeCurrent->pData;
	}
	// keys can be upper case
	return towlower((WCHAR)fn.key);
}

int Dictionary::ReGenerate(size_t pos, AnagramResult& rslt, CharacterRack& rack, AnagramSolutions& s)
{
	int err;
	TRI_FILE_NODE fn;
	WCHAR c;
	size_t rackPosition;

	if (rack.IsEmpty()) // no more letters to use
		return 0;
	c = GetNode(pos, fn); if (c == 0){ return 0; }

	if (rack.Exists(c, rackPosition)){
		rack.Cross(rackPosition);
		if (fn.data_pos){
			// this is a word in the dictionary
			rslt.AppendAnagram(rack.CurrentWord());
		}
		// go down the tree
		ReGenerate(fn.m_pos, rslt, rack, s);
		rack.Uncross(c, rackPosition);
	}

	// this key is not on the rack, so leave it all to the left and right nodes
	err = ReGenerate(fn.l_pos, rslt, rack, s);
	if (err != 0)
		return err;
	err = ReGenerate(fn.r_pos, rslt, rack, s);
	if (err != 0)
		return err;
	return 0;
}

bool SortByLen(const String& s1, const String& s2)
{
	return s1.size() > s2.size();
}

//
//	Combine and ReMakeCombines
//	----------------------------------------
//	Combine generated words into 'sentences'. The idea is simple: take a list of words that can be created from the letters in the original
//	input, and try to combine these words in ways so that as many as possible of the letters are used. The number of tests required by
//  brute force is N!, so we need to make some short cuts ...
//
int Dictionary::Combine(CharacterRack& rack, AnagramResult& lst_partial, AnagramResult& lst_out)
{
	StringArray vs;
	StringList::iterator it;

	if (m_pTree == NULL)
		return 2;

	for (it = lst_partial.begin(); it != lst_partial.end(); it++){
		vs.push_back((*it).c_str());
	}
	std::sort(vs.begin(), vs.end(), SortByLen);

	CombMap mapLetters;
	CombMap mapLengths;
	size_t i, j;

	unsigned previous_len = 0;
	for (i = 0; i < vs.size(); i++){
		unsigned len = vs[i].length();
		if (len != previous_len){
			mapLengths[len].push_back(i);
			previous_len = len;
		}
	}

	// map each unique letter to all strings where this letter can be found
	// are found
	for (i = 0; i < vs.size(); i++){
		for (j = 0; j < vs[i].size(); j++){
			mapLetters[vs[i][j]].push_back(i);
		}
	}
	// make each mapping just once
	for (CombMap::iterator it = mapLetters.begin(); it != mapLetters.end(); it++){
		std::sort((*it).second.begin(), (*it).second.end());
		(*it).second.erase(std::unique((*it).second.begin(), (*it).second.end()), (*it).second.end());
	}

	for (i = 0; i < vs.size(); i++){
		std::vector<size_t> positions;
		if (rack.Exists(vs[i], positions)){
			rack.Cross(positions);
			if (rack.IsEmpty()){
				// tombola!
				lst_out.AppendAnagram(rack.CurrentWord());
			}
			else{
				CharacterRack recRack(rack);
				AnagramResult recRslt;
				recRack.Shuffle(m_gen);
				if (ReMakeCombines(vs, i, mapLetters, recRack, recRslt)){
					recRslt.AppendWord(rack.CurrentWord());
					lst_out.MergeResult(recRslt);
				}
				if ((m_pfnCallback) && ((i % 50) == 49)){
					if (!m_pfnCallback(MulDiv(i, 100, vs.size()), m_dwCookie))
						break;
				}
			}
			rack.Uncross(vs[i], positions);
		}
		if (lst_out.size() >= m_maxResults)
			break;
	}

	return lst_out.empty() ? 0 : 1;
}

bool Dictionary::ReMakeCombines(StringArray& v, int idx, CombMap& mapLetters, CharacterRack& rack, AnagramResult& lst_out)
{
	// find first non-crossed letter
	// for all words with that letter try to cross out
	// if rack is empty, add to list and return
	// recursion
	int i, loc, size;
	String s;
	WCHAR cFirst;

	cFirst = rack.FindFirstNonCrossed();
	if(cFirst == '\0') // should never end up here with empty rack
		throw 92;

	if (mapLetters.find(cFirst) == mapLetters.end()){
		// this letter is not in the map, probably since é and e are the same in calls to wcscoll
		return false;
	}
	size = mapLetters[cFirst].size();
	for (i = 0; i < size; i++){
		loc = mapLetters[cFirst][i];
		if (loc < idx)
			continue;
		s = v[loc];
		std::vector<size_t> positions;
		if (rack.Exists(s, positions)){
			rack.Cross(positions);
			if (rack.IsEmpty()){
				// hurrah!
				lst_out.AppendAnagram(rack.CurrentWord());
				rack.Uncross(s, positions);
				return true;
			}
			CharacterRack recRack(rack);
			recRack.Shuffle(m_gen);
			if (ReMakeCombines(v, i, mapLetters, recRack, lst_out)){
				lst_out.AppendWord(rack.CurrentWord());
				rack.Uncross(s, positions);
				return true;
			}
			rack.Uncross(s, positions);
		}
	}
	return false;
}

#include "stdafx.h"
#include "application.h"
#include "anagrams.h"
#include "dictionary.h"
#include <fzindex.h>
#include <string>
#include <map>
#include <algorithm>
#include <iostream>

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

Dictionary::DICTIONARY_FILE_TYPE Dictionary::DetectFileType(String strPath)
{
	size_t n;
	FILE* fp;
	unsigned char szBuf[1024];
	int tst = IS_TEXT_UNICODE_UNICODE_MASK;
	size_t rs;

	CHAR szNameMB[MAX_PATH];
	size_t nConverted;
	wcstombs_s(&nConverted, szNameMB, MAX_PATH, strPath.c_str(), MAX_PATH);

	if (TstCheckFileHeader(szNameMB) == TST_ERROR_SUCCESS)
		return DFT_BINARY;

	if (fopen_s(&fp, szNameMB, "rb") != 0){
		return DFT_NONE;
	}

	// ugly hack, but works for a lot of file types
	memset(szBuf, 0, sizeof(szBuf));
	rs = fread(szBuf, 1, 1024, fp);
	fclose(fp);

	if (IsTextUnicode(szBuf, rs, &tst)){
		// IsTextUnicode isn't the most
		// stable of the Win32 functions ...
		if (wcslen((wchar_t*)szBuf) < (rs / sizeof(wchar_t)))
			return DFT_NONE;
		return DFT_TEXT;
	}

	for (n = 0; n < rs; n++){
		if (szBuf[n] < 9){
			return DFT_NONE;
		}
	}

	return DFT_TEXT;
}

DICT_ERROR Dictionary::ReadBinaryFile(String strPath)
{
	long rslt;
	TstDestroyTree(m_pTree);
	m_pTree = TstInitTree();

	CHAR szNameMB[MAX_PATH];
	size_t nConverted;
	wcstombs_s(&nConverted, szNameMB, MAX_PATH, strPath.c_str(), MAX_PATH);

	rslt = TstAttachToFile(m_pTree, szNameMB, TRUE);
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


DICT_ERROR Dictionary::ReadTextFile(String strPath)
{
	wchar_t buf[2048];
	FILE* fp;

	if (_tfopen_s(&fp, strPath.c_str(), _T("rb")) != 0)
		return DE_FAILED_OPEN;

	TstDestroyTree(m_pTree);
	m_pTree = TstInitTree();

	/* test for Unicode signatures */
	size_t r;
	int tst;

	r = fread(buf, 1, 512, fp);

	if (!IsTextUnicode(buf, r, &tst)){
		/* text mode will do ANSI->Unicode conversion */
		fclose(fp);
		_tfopen_s(&fp, strPath.c_str(), _T("r"));
	}
	else{
		/* binary mode will do no conversion */
		rewind(fp);
		if (tst & IS_TEXT_UNICODE_SIGNATURE){
			fread(buf, 2, 1, fp);
		}
	}

	while (!feof(fp)){
		if (!fgetws(buf, 2048, fp))
			break;
		if (!ParseTextLine(buf)){
			fclose(fp);
			return DE_BAD_FORMAT;
		}
	}

	fclose(fp);
	return DE_NONE;
}


bool Dictionary::ParseTextLine(const wchar_t* pszLine)
{
	// TODO: use isalpha or similar instead
	static LPCWSTR kszSeparators = L" .;,:!?\"#¤%&/()=\\+}{}[]$£@0123456789\t\r\n";
	// should do this with stl, but who knows
	// what to use?
	LPWSTR pszCopy;
	LPWSTR pszTok;
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
	return TRUE;
}

int Dictionary::ReFindFirstCompleteAnagram(size_t pos, AnagramResult& lst_out, CharacterRack& rack)
{
	TRI_FILE_NODE fn;
	size_t rackPos;
	WCHAR c = GetNode(pos, fn);
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
	bool fBreak = FALSE;
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

bool SortByLen(String& s1, String& s2)
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
	_ASSERT(cFirst != '\0'); // should never end up here with empty rack

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

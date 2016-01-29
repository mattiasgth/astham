#include <algorithm>
#include <random>

class CharacterRack : public std::wstring
{
public:
	CharacterRack(const CharacterRack& r)
		: std::wstring(r), m_strCanvas(L"")
	{
	}
	CharacterRack(String str)
	{
		for (iterator it = str.begin(); it != str.end(); it++){
			if (iswalpha(*it))
				push_back(*it);
		}
	}
	String MakeRandomUnique(std::mt19937& gen)
	{
		String strRslt;
		for (iterator it = begin(); it != end(); it++){
			if (*it != '#')
				strRslt.push_back(*it);
		}
		std::sort(strRslt.begin(), strRslt.end());
		strRslt.erase(std::unique(strRslt.begin(), strRslt.end()), strRslt.end());
		std::shuffle(strRslt.begin(), strRslt.end(), gen);
		*this = strRslt;
		return *this;
	}
	WCHAR FindFirstNonCrossed()
	{
		for (iterator it = begin(); it != end(); it++){
			if (*it != '#')
				return *it;
		}
		return '\0';
	}
	bool IsEmpty()
	{
		for (String::iterator it = begin(); it != end(); it++){
			if (*it != '#')
				return false;
		}
		return true;
	}
	bool Exists(WCHAR c, size_t& posOut)
	{
		WCHAR b1[2];
		WCHAR b2[2];
		b1[0] = c;
		b1[1] = '\0';
		b2[1] = '\0';
		size_t len = length();
		for (size_t pos = 0; pos < len; pos++){
			b2[0] = at(pos);
			if (wcscoll(b1, b2) == 0){
				posOut = pos;
				return true;
			}
		}
		return false;
	}
	bool Exists(String& s, std::vector<size_t>& posOut)
	{
		std::vector<size_t> rsltPos;
		CharacterRack cpy(*this);
		for (String::iterator it = s.begin(); it != s.end(); it++){
			size_t charPos;
			if (!cpy.Exists(*it, charPos)){
				return false;
			}
			cpy.Cross(charPos);
			rsltPos.push_back(charPos);
		}
		posOut = rsltPos;
		return true;
	}
	template <typename T> void Shuffle(T& gen)
	{
		std::shuffle(begin(), end(), gen);
	}
	template<typename T> void Cross(T& pos)
	{
		for (T::iterator it = pos.begin(); it != pos.end(); it++){
			Cross(*it);
		}
	}
	void Cross(size_t pos)
	{
		m_strCanvas.push_back(at(pos));
		at(pos) = '#';
	}
	template<typename T> void Uncross(String& s, T& pos)
	{
		int idx = 0;
		for (T::iterator it = pos.begin(); it != pos.end(); it++){
			Uncross(s[idx++], *it);
		}
	}
	void Uncross(WCHAR c, size_t pos)
	{
		at(pos) = c;
		m_strCanvas.pop_back();
	}
	String& CurrentWord(){ return m_strCanvas; };
protected:
	std::wstring m_strCanvas;
};

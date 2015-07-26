#pragma once

namespace supermarx
{

inline void str_replace(std::string& s,  std::string const& search, std::string const& replace)
{
	for(size_t pos = 0; ; pos += replace.length())
	{
		pos = s.find(search, pos);
		if(pos == std::string::npos)
			break;

		s.erase(pos, search.length());
		s.insert(pos, replace);
	}
}

inline void remove_hyphens(std::string& str)
{
	str_replace(str, "\u00AD", ""); // Remove soft hyphen
}

}

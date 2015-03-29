#pragma once

#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace supermarx
{
	class util
	{
		util() = delete;
		util(util&) = delete;
		void operator=(util&) = delete;
		
	public:
		static inline std::string sanitize(const std::string& str)
		{
			std::stringstream is(str);
			std::string result;
			
			while(is.peek() != std::char_traits<char>::eof())
			{
				std::string tmp;
				is >> tmp;
				
				if(!result.empty())
					result.append(" ");
				
				result.append(tmp);
			}
			
			boost::trim(result);
			return result;
		}

		static inline bool contains_attr(const std::string& needle, const std::string& haystack)
		{
			static const boost::regex regex_classes(" ");

			boost::sregex_token_iterator it(haystack.begin(), haystack.end(), regex_classes, -1);
			boost::sregex_token_iterator end;

			while(it != end)
			{
				auto m = *it++;
				if(m.matched && m.compare(needle) == 0)
					return true;
			}

			return false;
		}
	};
}

#pragma once

#include <string>
#include <boost/algorithm/string.hpp>

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
	};
}

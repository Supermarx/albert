#pragma once

#include <string>
#include <functional>

#include "html_parser.hpp"

namespace supermarx
{
	class html_recorder
	{
	public:
		typedef std::function<void(std::string ch)> callback_t;
	
	private:
		std::string str;
		size_t depth;
		callback_t f;
		
	public:
		html_recorder(callback_t f)
		: str()
		, depth(0)
		, f(f)
		{}
		
		void startElement()
		{
			depth++;
		}
		
		void characters(const std::string& ch)
		{
			str.append(ch);
		}
		
		bool endElement()
		{
			if(depth == 0)
			{
				f(str);
				return true;
			}
			
			depth--;
			return false;
		}
		
	};
}

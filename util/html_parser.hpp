#pragma once

#include <istream>
#include <SAX/helpers/DefaultHandler.hpp>

namespace supermarx
{
	class html_parser
	{
	public:
		typedef Arabica::SAX::DefaultHandler<std::string> default_handler;
	
		html_parser() = delete;
		
		static void parse(const std::string& src, default_handler& p);
		static void parse(std::istream& is, default_handler& p);
	};
}

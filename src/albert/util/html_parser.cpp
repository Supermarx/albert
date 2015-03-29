#include "html_parser.hpp"

#include <Taggle/Taggle.hpp>

namespace supermarx
{
	void html_parser::parse(const std::string& src, html_parser::default_handler& p)
	{
		std::istringstream ss(src);
		parse(ss, p);
	}
	
	void html_parser::parse(std::istream& is, html_parser::default_handler& p)
	{
		Arabica::SAX::Taggle<std::string> parser;

		parser.setContentHandler(p);
		parser.setErrorHandler(p);

		Arabica::SAX::InputSource<std::string> i(is);
		parser.parse(i);
	}
}

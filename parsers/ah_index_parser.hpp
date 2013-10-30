#pragma once

#include <boost/lexical_cast.hpp>

#include "../util/html_parser.hpp"

namespace allerhande
{
	class ah_index_parser : public html_parser::default_handler
	{
	public:
		typedef std::function<void(uint64_t)> callback_t;
	
	private:
		callback_t f;
	
	public:
		ah_index_parser(callback_t f)
		: f(f)
		{}
	
		template<typename T>
		void parse(T source)
		{
			html_parser::parse(source, *this);
		}
	
		virtual void startElement(const std::string& /* namespaceURI */, const std::string& /* localName */, const std::string& qName, const AttributesT& atts)
		{
			if(qName != "a")
				return;
			
			std::string recipe_id = atts.getValue("data-recipe-id");
			
			if(recipe_id == "")
				return;
			
			f(boost::lexical_cast<uint64_t>(recipe_id));
		}
	};
}

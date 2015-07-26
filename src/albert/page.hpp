#pragma once

#include <string>
#include <supermarx/message/tag.hpp>

namespace supermarx
{

struct page_t
{
	std::string uri;
	std::string name;

	bool expand;
	std::vector<message::tag> tags;
};

}

#include "scraper.hpp"

int main(int argc, char** argv)
{
	supermarx::scraper s([&](const supermarx::product&) {});
	s.scrape();
}

#include "scraper.hpp"

#include <vector>
#include <iostream>

int main()
{
	supermarx::scraper s([&](const supermarx::product& product) {
		std::cout << "FANTASTISCHE " << product.name << " voor maar " << product.price_in_cents/100.0f << std::endl;
	});

	s.scrape();
}

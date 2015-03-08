#include "scraper.hpp"

#include <vector>
#include <iostream>

int main()
{
	supermarx::scraper s([&](const supermarx::product& product) {
		std::cout << "Product '" << product.name << "' voor " << product.price_in_cents/100.0f << " EUR" << std::endl;
	});

	s.scrape();
}

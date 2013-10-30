#include "scraper.hpp"

#include <vector>
#include <iostream>

int main()
{
	std::vector<supermarx::product> products;
	supermarx::scraper s([&](const supermarx::product& product) {
		products.push_back(product);
	});

	s.scrape();

	for(supermarx::product& product : products){
		std::cout << "FANTASTISCHE " << product.name << " voor maar " << product.price_in_cents/100.0f << std::endl;
	}
}

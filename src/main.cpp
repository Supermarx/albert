#include "scraper.hpp"

#include <vector>
#include <iostream>

int main()
{
	std::vector<supermarx::Product> products;
	supermarx::scraper s([&](const supermarx::Product& product) {
		products.push_back(product);
	});

	s.scrape();

	for(supermarx::Product& product : products){
		std::cout << "FANTASTISCHE " << product.name << " voor maar " << product.price_in_cents/100.0f << std::endl;
	}
}

#include <iostream>

#include "json_reader.h"
#include "transport_catalogue.h"

int main() {
    transport_catalogue::TransportCatalogue catalogue;
    json_reader::Requests(catalogue, std::cin, std::cout);
}
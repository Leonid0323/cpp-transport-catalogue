#include <iostream>

#include "json_reader.h"
#include "transport_catalogue.h"

int main() {
    transport_catalogue::TransportCatalogue catalogue;
    json_reader::JSONReader json_read(catalogue);
    json_read.Requests(std::cin, std::cout);
}
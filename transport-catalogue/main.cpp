#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"

using namespace std;

int main() {
    transport_catalogue::TransportCatalogue catalogue;

    input_reader::InputData(catalogue, cin);

    stat_reader::IntputRequest(catalogue, cin, cout);
}
#pragma once

#include<algorithm>
#include <iosfwd>
#include <iomanip>
#include <iostream>
#include <set>
#include <string_view>
#include <unordered_set>

#include "transport_catalogue.h"

using namespace std::literals;


namespace stat_reader{

struct Request{
    std::string_view command;      
    std::string_view id;
};

void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue, 
                       std::string_view request, std::ostream& output);

void IntputRequest(transport_catalogue::TransportCatalogue& catalogue, 
                   std::istream& input, std::ostream& output);
    
}
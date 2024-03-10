#pragma once

#include<algorithm>
#include <iosfwd>
#include <iomanip>
#include <iostream>
#include <set>
#include <string_view>
#include <unordered_set>

#include "json.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include  "geo.h"
#include "map_renderer.h"

using namespace std::literals;

namespace json_reader{
    void Requests(transport_catalogue::TransportCatalogue& catalogue, std::istream& input, std::ostream& output);
}
#pragma once

#include<algorithm>
#include <iosfwd>
#include <iomanip>
#include <iostream>
#include <set>
#include <string_view>
#include <unordered_set>

#include "json.h"
#include "transport_catalogue.h"
#include  "geo.h"
#include "map_renderer.h"

using namespace std::literals;

namespace json_reader{
    

    class JSONReader{
public:
    explicit JSONReader(transport_catalogue::TransportCatalogue& catalogue)
        :catalogue_(catalogue){
        }
    transport_catalogue::TransportCatalogue& GetCatalouge() const {
        return catalogue_;
    }
    void Requests(std::istream& input, std::ostream& output);
    
    
private:
    transport_catalogue::TransportCatalogue& catalogue_;
    
    std::vector<std::string_view> ParseRoute(const json::Array& stops, bool is_roundtrip);
    
    void BaseRequests(json::Array& base_requests);
    
    double CalculateGeographyLength(const std::vector<const transport_catalogue::Stop*>& stops);
    int CalculateRouteLength(const std::vector<const transport_catalogue::Stop*>& stops);
    
    json::Node BusInfo(json::Dict request);
    json::Node StopInfo(json::Dict request);
    
    json::Document StatRequests(json::Array& stat_requests, json::Array base_requests,
                                const map_renderer::Mapping& mapping);
};
}
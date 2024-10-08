#pragma once
#include<algorithm>
#include<deque>
#include<string>
#include<string_view>
#include<unordered_map>
#include<set>
#include<vector>

#include "geo.h"

using namespace std::literals;


namespace transport_catalogue{
    
struct Stop{
    std::string name;
    geo::Coordinates coord;
};

struct Bus{
    std::string name;
    std::vector<const Stop*> stops;
}; 

struct StopDistanceHasher {
    size_t operator()(const std::pair<const Stop*, const Stop*>& distance) const {
        size_t first_hash = std::hash<const void*>{}(distance.first);
        size_t second_hash = std::hash<const void*>{}(distance.second);
        return first_hash + second_hash*37;
    }
};

class TransportCatalogue {
public:
    void AddBus(std::string&& busname, const std::vector<std::string_view>& stops);
    
    void AddStop(std::string&& stopname, geo::Coordinates coordinates);
   
    const Bus* SearchBus(std::string_view busname) const;
    
    const Stop* SearchStop(std::string_view stopname) const;
    
    std::vector<const Stop*> GetInfoAboutBus(std::string_view busname) const;
    
    std::set<const Bus*> GetInfoAboutStop(std::string_view stopname) const;

    void AddDistanceStops(std::string_view lhs, std::string_view rhs, int distance);

    int GetDistanceStops(std::string_view lhs, std::string_view rhs) const;
    
private:
    
    
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, std::set<const Bus*>> stopname_to_bus_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, const Bus*> busname_to_stop_;
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopDistanceHasher> distance_;
};

}
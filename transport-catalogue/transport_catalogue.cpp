#include "transport_catalogue.h"


namespace transport_catalogue{
    
void TransportCatalogue::AddBus(std::string&& busname, const std::vector<std::string_view>& stops){
    Bus bus;
    bus.name = std::move(busname);
    for (const auto& stopname : stops) {
        if (stopname_to_stop_.count(stopname)) {
            bus.stops.push_back(stopname_to_stop_.at(stopname));
        }
    }
    buses_.push_back(std::move(bus));
    busname_to_stop_[buses_.back().name] = &buses_.back();
    for (const Stop* stopname : buses_.back().stops){
        stopname_to_bus_[(*stopname).name].insert(&buses_.back());
    }
}

void TransportCatalogue::AddStop(std::string&& stopname, geo::Coordinates coordinates){
    Stop stop = {stopname, coordinates};
    stops_.push_back(std::move(stop));
    stopname_to_stop_[stops_.back().name] = &stops_.back();
    stopname_to_bus_[stops_.back().name] = {};
}

const Bus* TransportCatalogue::SearchBus(std::string_view busname) const{
    
    if(!busname_to_stop_.count(busname)){
        return nullptr;
    }
    return busname_to_stop_.at(busname);
}

const Stop* TransportCatalogue::SearchStop(std::string_view stopname) const{ 
    if(!stopname_to_stop_.count(stopname)){
        return nullptr;
    }
    return stopname_to_stop_.at(stopname);
}

std::vector<const Stop*> TransportCatalogue::GetInfoAboutBus(std::string_view busname) const{
    const Bus* pbus = SearchBus(busname);
    if (pbus != nullptr){
        return (*pbus).stops;
    }
    return {};
}

std::set<const Bus*> TransportCatalogue::GetInfoAboutStop(std::string_view stopname) const{
    if (stopname_to_bus_.count(stopname)){
        if (stopname_to_bus_.at(stopname).empty()){
            return {nullptr};
        }
        return stopname_to_bus_.at(stopname);
    }
    return {};
}

void TransportCatalogue::AddDistanceStops(std::string_view lhs, std::string_view rhs, int distance) {
    distance_[{SearchStop(lhs), SearchStop(rhs)}] = distance;
}

int TransportCatalogue::GetDistanceStops(std::string_view lhs, std::string_view rhs) const {
    if (distance_.count({ SearchStop(lhs) , SearchStop(rhs) })) {
        return distance_.at({ SearchStop(lhs) , SearchStop(rhs) });
    }
    return distance_.at({ SearchStop(rhs) , SearchStop(lhs) });
}

}
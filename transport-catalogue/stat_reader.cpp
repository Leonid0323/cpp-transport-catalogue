#include "stat_reader.h"

namespace stat_reader{

using namespace transport_catalogue;
    
namespace detail{
    
bool CompareSortBus(const Bus* lhs, const Bus* rhs){
    return (*lhs).name < (*rhs).name;
}

Request ParseRequest(std::string_view request){
    size_t space = request.find(' ');
    Request parsed_request;
    parsed_request.command = request.substr(0, space);
    parsed_request.id = request.substr(space+1);
    return parsed_request;
}

double CalculateGeographyLength(const std::vector<const Stop*>& stops){
    double length = 0;
    for(size_t i = 0; i+1!= stops.size();++i){
        length +=ComputeDistance((*(stops[i])).coord, (*(stops[i+1])).coord);
    }
    return length;
}

int CalculateRouteLength(const TransportCatalogue& transport_catalogue, const std::vector<const Stop*>& stops) {
    int length = 0;
    for (size_t i = 0; i+1 != stops.size(); ++i) {
        length += transport_catalogue.GetDistanceStops((*(stops[i])).name, (*(stops[i + 1])).name);
    }
    return length;
}

void PrintStopInfo(const TransportCatalogue& transport_catalogue, 
                   std::string_view stopname, std::ostream& output){
    std::set<const Bus*> buses = transport_catalogue.GetInfoAboutStop(stopname);
    if(buses.empty()){
        output << "Stop "s << stopname << ": not found"s << std::endl;
    } else if(buses.count(nullptr)){
        output << "Stop "s << stopname << ": no buses"s << std::endl;
    } else{
        std::vector<const Bus*> vec_buses(buses.begin(),buses.end());
        std::sort(vec_buses.begin(), vec_buses.end(), CompareSortBus);
        output << "Stop "s << stopname << ": buses "s;
        bool is_first = true;
        for (const Bus* bus : vec_buses) {
            if (!is_first) {
                output << " "s;
            }
            output << (*(bus)).name;
            is_first = false;
        }
        output << std::endl;
    }
}

void PrintBusInfo(const TransportCatalogue& transport_catalogue, 
                  std::string_view busname, std::ostream& output){
    std::vector<const Stop*> stops = transport_catalogue.GetInfoAboutBus(busname);
    if (stops.empty()){
        output << "Bus "s << busname << ": not found"s << std::endl;
    } else{
        output << "Bus "s << busname << ": "s << stops.size() << " stops on route, "s;
        std::unordered_set uset_stops(stops.begin(), stops.end());
        output << uset_stops.size() << " unique stops, "s;
        int length = CalculateRouteLength(transport_catalogue, stops);
        double geography_length = CalculateGeographyLength(stops);
        output << std::setprecision(6) << length << " route length, "s
            << std::setprecision(6) << static_cast<double>(length)/geography_length << " curvature"s << std::endl;
    }
}

}    
    
void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue, 
                       std::string_view request, std::ostream& output) {
    Request parsed_request = detail::ParseRequest(request);
    if(parsed_request.command=="Bus"s){
         detail::PrintBusInfo(transport_catalogue, parsed_request.id, output);
    } else{
        detail::PrintStopInfo(transport_catalogue, parsed_request.id, output);
    }
}
    
void IntputRequest(transport_catalogue::TransportCatalogue& catalogue, 
                   std::istream& input, std::ostream& output){
    int stat_request_count;
    input >> stat_request_count >> std::ws;
    for (int i = 0; i < stat_request_count; ++i) {
        std::string line;
        std::getline(input, line);
        stat_reader::ParseAndPrintStat(catalogue, line, output);
    }
}
    
}
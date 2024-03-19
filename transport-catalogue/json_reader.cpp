#include "json_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>


namespace json_reader{
    
    std::vector<std::string_view> JSONReader::ParseRoute(const json::Array& stops, bool is_roundtrip){
        std::vector<std::string_view> results;
        for(auto& stop_node: stops){
            std::string_view stop = stop_node.AsString();;
            results.push_back(stop);
        }
        if(!is_roundtrip){
            results.insert(results.end(), std::next(results.rbegin()), results.rend());
        }
        return results;
    }
    
    void JSONReader::BaseRequests(json::Array& base_requests){
        std::vector<size_t> buses;
        std::vector< std::pair<std::string, json::Dict>> from_stop_to_stop_distance;
        for(size_t i = 0; i !=base_requests.size(); ++i){
            json::Dict request = base_requests[i].AsMap();
            if (request["type"s].AsString() == "Stop"s){
                std::string name = request["name"s].AsString(); 
                if(auto road_distances (request["road_distances"s].AsMap()); !road_distances.empty()){
                    from_stop_to_stop_distance.push_back({name, road_distances});
                }
                catalogue_.AddStop(std::move(name), {request["latitude"s].AsDouble(), request["longitude"s].AsDouble()});
            } else{
                buses.push_back(i);
            }
        }
        
        for (const auto& [from, all_to_and_distance] : from_stop_to_stop_distance) {
            for (const auto& [to, distance] : all_to_and_distance){
                catalogue_.AddDistanceStops(from, to, distance.AsInt());
            }
        }
        
        for (size_t i : buses){
            json::Dict request = base_requests[i].AsMap();
            std::string name = request["name"s].AsString();
            catalogue_.AddBus(std::move(name), ParseRoute(request["stops"s].AsArray(), request["is_roundtrip"s].AsBool()));
        }
    }

    bool CompareSortBus(const transport_catalogue::Bus* lhs, const transport_catalogue::Bus* rhs){
        return (*lhs).name < (*rhs).name;
    }
    
    double JSONReader::CalculateGeographyLength(const std::vector<const transport_catalogue::Stop*>& stops){
        double length = 0;
        for(size_t i = 0; i+1!= stops.size();++i){
            length +=ComputeDistance((*(stops[i])).coord, (*(stops[i+1])).coord);
        }
        return length;
    }

    int JSONReader::CalculateRouteLength(const std::vector<const transport_catalogue::Stop*>& stops) {
        int length = 0;
        for (size_t i = 0; i+1 != stops.size(); ++i) {
            length += catalogue_.GetDistanceStops((*(stops[i])).name, (*(stops[i + 1])).name);
        }
        return length;
    }
    
    json::Node JSONReader::BusInfo(json::Dict request){
        json::Dict result;
        const transport_catalogue::Bus* bus = catalogue_.SearchBus(request["name"s].AsString());
        if(bus==nullptr){
            result = { {"request_id"s, request["id"s]}, {"error_message"s, json::Node{"not found"s}}};
        }
         else {
            std::vector<const transport_catalogue::Stop*> stops = catalogue_.GetInfoAboutBus(request["name"].AsString());
            std::unordered_set uset_stops(stops.begin(), stops.end());
            int length = CalculateRouteLength(stops);
            double geography_length = CalculateGeographyLength(stops);
            double curvature = static_cast<double>(length)/geography_length;
           
            
            result = { {"curvature"s, json::Node{curvature}}, {"request_id"s, request["id"s]}, {"route_length"s, json::Node{length}},
            {"stop_count"s, json::Node{static_cast<int>(stops.size())}}, 
            {"unique_stop_count"s, json::Node{static_cast<int>(uset_stops.size())}} };
        }
        return json::Node{result};
    }
    
    json::Node JSONReader::StopInfo(json::Dict request){
        json::Dict result;
        const transport_catalogue::Stop* stop = catalogue_.SearchStop(request["name"s].AsString());
        if(stop==nullptr){
            result = { {"request_id"s, request["id"s]}, {"error_message"s, json::Node{"not found"s}} };
        } else{
            std::set<const transport_catalogue::Bus*> buses = catalogue_.GetInfoAboutStop(request["name"].AsString());
            json::Array arr_buses;
            if(!buses.empty()){
                std::vector<const transport_catalogue::Bus*> vec_buses(buses.begin(),buses.end());
                std::sort(vec_buses.begin(), vec_buses.end(), CompareSortBus);
                for(const auto bus:vec_buses){
                    arr_buses.push_back(json::Node{bus->name});
                }
            }
            result = { {"request_id"s, request["id"s]}, {"buses"s, json::Node{arr_buses}} };
        }
        return json::Node{result};
    }
    
    json::Document JSONReader::StatRequests(json::Array& stat_requests, json::Array base_requests,
                                const map_renderer::Mapping& mapping){
        json::Array result;
        for(size_t i = 0; i !=stat_requests.size(); ++i){
            json::Node req;
            json::Dict request = stat_requests[i].AsMap();
            if (request["type"s].AsString() == "Bus"s){
                req = BusInfo(request);
            } else if(request["type"s].AsString() == "Stop"s){
                req = StopInfo(request); 
            } else{
                req = map_renderer::DrawRoute(GetCatalouge(), base_requests, mapping, request);
            }
            result.push_back(req);
        }
        return json::Document{json::Node{result}};
    }
    
    void JSONReader::Requests(std::istream& input, std::ostream& output){
        auto node = json::Load(input).GetRoot();
        json::Dict requests =  node.AsMap();
        json::Array base_requests = requests["base_requests"s].AsArray();
        BaseRequests(base_requests);
        json::Dict render_settings = requests["render_settings"s].AsMap();
        map_renderer::Mapping mapping = map_renderer::RenderSettings(render_settings);
        json::Array stat_requests = requests["stat_requests"s].AsArray();
        json::Document data = StatRequests(stat_requests, base_requests, mapping);
        json::Print(data, output);
    }
}
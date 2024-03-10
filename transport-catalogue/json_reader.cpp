#include "json_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>


namespace json_reader{
    
    std::vector<std::string_view> ParseRoute(const json::Array& stops, bool is_roundtrip){
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
    
    void BaseRequests(transport_catalogue::TransportCatalogue& catalogue, json::Array& base_requests){
        std::vector<size_t> buses;
        std::vector< std::pair<std::string, json::Dict>> from_stop_to_stop_distance;
        for(size_t i = 0; i !=base_requests.size(); ++i){
            json::Dict request = base_requests[i].AsMap();
            if (request["type"s].AsString() == "Stop"s){
                std::string name = request["name"s].AsString(); 
                if(auto road_distances (request["road_distances"s].AsMap()); !road_distances.empty()){
                    from_stop_to_stop_distance.push_back({name, road_distances});
                }
                catalogue.AddStop(std::move(name), {request["latitude"s].AsDouble(), request["longitude"s].AsDouble()});
            } else{
                buses.push_back(i);
            }
        }
        
        for (const auto& [from, all_to_and_distance] : from_stop_to_stop_distance) {
            for (const auto& [to, distance] : all_to_and_distance){
                catalogue.AddDistanceStops(from, to, distance.AsInt());
            }
        }
        
        for (size_t i : buses){
            json::Dict request = base_requests[i].AsMap();
            std::string name = request["name"s].AsString();
            catalogue.AddBus(std::move(name), ParseRoute(request["stops"s].AsArray(), request["is_roundtrip"s].AsBool()));
        }
    }
    
    svg::Color GetColor(json::Node color_node){
        svg::Color result;
        if(color_node.IsString()){
            result = color_node.AsString();
        } else if(color_node.AsArray().size()==3){
            result = svg::Rgb{static_cast<uint16_t>(color_node.AsArray()[0].AsInt()),
                    static_cast<uint16_t>(color_node.AsArray()[1].AsInt()),
                    static_cast<uint16_t>(color_node.AsArray()[2].AsInt())};
        } else{
            result = svg::Rgba{static_cast<uint16_t>(color_node.AsArray()[0].AsInt()),
                    static_cast<uint16_t>(color_node.AsArray()[1].AsInt()),
                    static_cast<uint16_t>(color_node.AsArray()[2].AsInt()),
                    color_node.AsArray()[3].AsDouble()};
        }
        return result;
    }
    
    map_renderer::Mapping RenderSettings(json::Dict render_settings){
        map_renderer::Mapping result;
        result.width = render_settings["width"s].AsDouble();
        result.height = render_settings["height"s].AsDouble();
        result.padding = render_settings["padding"s].AsDouble();
        result.line_width = render_settings["line_width"s].AsDouble();
        result.stop_radius = render_settings["stop_radius"s].AsDouble();
        result.bus_label_font_size = render_settings["bus_label_font_size"s].AsInt();
        result.bus_label_offset.first = render_settings["bus_label_offset"s].AsArray()[0].AsDouble();
        result.bus_label_offset.second = render_settings["bus_label_offset"s].AsArray()[1].AsDouble();
        result.stop_label_font_size = render_settings["stop_label_font_size"s].AsInt();
        result.stop_label_offset.first = render_settings["stop_label_offset"s].AsArray()[0].AsDouble();
        result.stop_label_offset.second = render_settings["stop_label_offset"s].AsArray()[1].AsDouble();
        result.underlayer_color = GetColor(render_settings["underlayer_color"s]);
        result.underlayer_width = render_settings["underlayer_width"s].AsDouble();
        json::Array color_palette = render_settings["color_palette"s].AsArray();
        
        for(const auto& color:color_palette){
            result.color_palette.push_back(GetColor(color));
        }
        return result;
    }
    
    svg::Polyline GetBusRoute(const std::vector<const transport_catalogue::Stop*>& stops, const map_renderer::SphereProjector proj){
        std::vector<geo::Coordinates> geo_coords_stop;
        
        for(const auto stop: stops){
            geo_coords_stop.push_back(stop->coord);
        }
        
        svg::Polyline polyline;
        
        for (const auto& geo_coord: geo_coords_stop) {
            const svg::Point screen_coord = proj(geo_coord);
            polyline.AddPoint(screen_coord);
        }
        
        return polyline;  
    }

    json::Node DrawRoute(transport_catalogue::TransportCatalogue& catalogue, json::Array base_requests, 
                        const map_renderer::Mapping& mapping, json::Dict request){
        std::vector<std::pair<std::string, bool>> buses;
        
        for(size_t i = 0; i !=base_requests.size(); ++i){
            json::Dict request = base_requests[i].AsMap();
            if (request["type"s].AsString() == "Bus"s){
                buses.push_back({request["name"s].AsString(), request["is_roundtrip"s].AsBool()});
            }
        }
        
        std::sort(buses.begin(), buses.end());
        
        const double WIDTH = mapping.width;
        const double HEIGHT = mapping.height;
        const double PADDING = mapping.padding;
        
        std::vector<const transport_catalogue::Stop*> all_stops;
        for(const auto& bus: buses){
            std::vector<const transport_catalogue::Stop*> stops = catalogue.GetInfoAboutBus(bus.first);
            for(const auto stop: stops){
                if(std::find(all_stops.begin(), all_stops.end(), stop)== all_stops.end()){
                    all_stops.push_back(stop);
                }
            }
        }
        
        const map_renderer::SphereProjector proj{
            all_stops.begin(), all_stops.end(), WIDTH, HEIGHT, PADDING
        };
        
        size_t i = 0;
        svg::Document doc;
        for(const auto& [bus_name, bus_is_roundtrip]: buses){
            std::vector<const transport_catalogue::Stop*> stops = catalogue.GetInfoAboutBus(bus_name);
            if(!stops.empty()){
                const svg::Polyline polyline = GetBusRoute(stops, proj);
                
                doc.Add(svg::Polyline{polyline}
                        .SetStrokeColor(mapping.color_palette[i])
                        .SetFillColor("none"s)
                        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                        .SetStrokeWidth(mapping.line_width));
                
                i++;
                if(i == mapping.color_palette.size()){
                    i = 0;
                }
            }
            
        }
        
        i = 0;
        for(const auto& [bus_name, bus_is_roundtrip]: buses){
            std::vector<const transport_catalogue::Stop*> stops = catalogue.GetInfoAboutBus(bus_name);
            if(!stops.empty()){
                doc.Add(svg::Text()
                        .SetPosition(proj(stops.back()->coord))
                        .SetOffset({mapping.bus_label_offset.first, mapping.bus_label_offset.second})
                        .SetFontSize(mapping.bus_label_font_size)
                        .SetFontFamily("Verdana"s)
                        .SetFontWeight("bold")
                        .SetData(bus_name)
                        .SetStrokeColor(mapping.underlayer_color)
                        .SetFillColor(mapping.underlayer_color)
                        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                        .SetStrokeWidth(mapping.underlayer_width)); 
                
                doc.Add(svg::Text()
                        .SetPosition(proj(stops.back()->coord))
                        .SetOffset({mapping.bus_label_offset.first, mapping.bus_label_offset.second})
                        .SetFontSize(mapping.bus_label_font_size)
                        .SetFontFamily("Verdana"s)
                        .SetFontWeight("bold")
                        .SetData(bus_name)
                        .SetFillColor(mapping.color_palette[i]));
                
                if((!bus_is_roundtrip) && stops[stops.size()/2]->name != stops.back()->name){
                    doc.Add(svg::Text()
                            .SetPosition(proj(stops[stops.size()/2]->coord))
                            .SetOffset({mapping.bus_label_offset.first, mapping.bus_label_offset.second})
                            .SetFontSize(mapping.bus_label_font_size)
                            .SetFontFamily("Verdana"s)
                            .SetFontWeight("bold"s)
                            .SetData(bus_name)
                            .SetStrokeColor(mapping.underlayer_color)
                            .SetFillColor(mapping.underlayer_color)
                            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                            .SetStrokeWidth(mapping.underlayer_width));
                    
                    doc.Add(svg::Text()
                            .SetPosition(proj(stops[stops.size()/2]->coord))
                            .SetOffset({mapping.bus_label_offset.first, mapping.bus_label_offset.second})
                            .SetFontSize(mapping.bus_label_font_size)
                            .SetFontFamily("Verdana"s)
                            .SetFontWeight("bold"s)
                            .SetData(bus_name)
                            .SetFillColor(mapping.color_palette[i]));
                }
                i++;
                if(i == mapping.color_palette.size()){
                    i = 0;
                }
            }
            
        }
        
        std::sort(all_stops.begin(), all_stops.end(),
                    [](const transport_catalogue::Stop* lhs,const transport_catalogue::Stop* rhs){
                        return lhs->name < rhs->name; 
                    });
        
        for(const auto stop:all_stops){
            doc.Add(svg::Circle()
                    .SetCenter(proj(stop->coord))
                    .SetRadius(mapping.stop_radius)
                    .SetFillColor("white"s));
        }
        
        for(const auto stop:all_stops){
            doc.Add(svg::Text()
                    .SetPosition(proj(stop->coord))
                    .SetOffset({mapping.stop_label_offset.first, mapping.stop_label_offset.second})
                    .SetFontSize(mapping.stop_label_font_size)
                    .SetFontFamily("Verdana"s)
                    .SetData(stop->name)
                    .SetStrokeColor(mapping.underlayer_color)
                    .SetFillColor(mapping.underlayer_color)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeWidth(mapping.underlayer_width));
            
            doc.Add(svg::Text()
                    .SetPosition(proj(stop->coord))
                    .SetOffset({mapping.stop_label_offset.first, mapping.stop_label_offset.second})
                    .SetFontSize(mapping.stop_label_font_size)
                    .SetFontFamily("Verdana"s)
                    .SetData(stop->name)
                    .SetFillColor("black"s));
        }
        
        std::stringstream ss;
        doc.Render(ss);
        json::Dict result = {{"map"s, json::Node{ss.str()}},{"request_id"s, request["id"s]}};
        return json::Node{result};
    }

    bool CompareSortBus(const transport_catalogue::Bus* lhs, const transport_catalogue::Bus* rhs){
        return (*lhs).name < (*rhs).name;
    }
    
    double CalculateGeographyLength(const std::vector<const transport_catalogue::Stop*>& stops){
        double length = 0;
        for(size_t i = 0; i+1!= stops.size();++i){
            length +=ComputeDistance((*(stops[i])).coord, (*(stops[i+1])).coord);
        }
        return length;
    }

    int CalculateRouteLength(const transport_catalogue::TransportCatalogue& transport_catalogue, 
                             const std::vector<const transport_catalogue::Stop*>& stops) {
        int length = 0;
        for (size_t i = 0; i+1 != stops.size(); ++i) {
            length += transport_catalogue.GetDistanceStops((*(stops[i])).name, (*(stops[i + 1])).name);
        }
        return length;
    }
    
    json::Node BusInfo(transport_catalogue::TransportCatalogue& catalogue, json::Dict request){
        json::Dict result;
        const transport_catalogue::Bus* bus = catalogue.SearchBus(request["name"s].AsString());
        if(bus==nullptr){
            result = { {"request_id"s, request["id"s]}, {"error_message"s, json::Node{"not found"s}}};
        }
         else {
            std::vector<const transport_catalogue::Stop*> stops = catalogue.GetInfoAboutBus(request["name"].AsString());
            std::unordered_set uset_stops(stops.begin(), stops.end());
            int length = CalculateRouteLength(catalogue, stops);
            double geography_length = CalculateGeographyLength(stops);
            double curvature = static_cast<double>(length)/geography_length;
           
            
            result = { {"curvature"s, json::Node{curvature}}, {"request_id"s, request["id"s]}, {"route_length"s, json::Node{length}},
            {"stop_count"s, json::Node{static_cast<int>(stops.size())}}, 
            {"unique_stop_count"s, json::Node{static_cast<int>(uset_stops.size())}} };
        }
        return json::Node{result};
    }
    
    json::Node StopInfo(transport_catalogue::TransportCatalogue& catalogue, json::Dict request){
        json::Dict result;
        const transport_catalogue::Stop* stop = catalogue.SearchStop(request["name"s].AsString());
        if(stop==nullptr){
            result = { {"request_id"s, request["id"s]}, {"error_message"s, json::Node{"not found"s}} };
        } else{
            std::set<const transport_catalogue::Bus*> buses = catalogue.GetInfoAboutStop(request["name"].AsString());
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
    
    json::Document StatRequests(transport_catalogue::TransportCatalogue& catalogue, json::Array& stat_requests,
                                json::Array base_requests, const map_renderer::Mapping& mapping){
        json::Array result;
        for(size_t i = 0; i !=stat_requests.size(); ++i){
            json::Node req;
            json::Dict request = stat_requests[i].AsMap();
            if (request["type"s].AsString() == "Bus"s){
                req = BusInfo(catalogue, request);
            } else if(request["type"s].AsString() == "Stop"s){
                req = StopInfo(catalogue, request); 
            } else{
                req = DrawRoute(catalogue, base_requests, mapping, request);
            }
            result.push_back(req);
        }
        return json::Document{json::Node{result}};
    }
    
    void Requests(transport_catalogue::TransportCatalogue& catalogue, std::istream& input, std::ostream& output){
        auto node = json::Load(input).GetRoot();
        json::Dict requests =  node.AsMap();
        json::Array base_requests = requests["base_requests"s].AsArray();
        BaseRequests(catalogue, base_requests);
        json::Dict render_settings = requests["render_settings"s].AsMap();
        map_renderer::Mapping mapping = RenderSettings(render_settings);
        json::Array stat_requests = requests["stat_requests"s].AsArray();
        json::Document data = StatRequests(catalogue, stat_requests, base_requests, mapping);
        json::Print(data, output);
    }
}
#include "map_renderer.h"

namespace map_renderer{

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
    
Mapping RenderSettings(json::Dict render_settings){
    Mapping result;
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
    
svg::Polyline GetBusRoute(const std::vector<const transport_catalogue::Stop*>& stops, const SphereProjector proj){
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

json::Node DrawRoute(const transport_catalogue::TransportCatalogue& catalogue, json::Array base_requests, 
                        const Mapping& mapping, json::Dict request){
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
        
    const SphereProjector proj{
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
    
}
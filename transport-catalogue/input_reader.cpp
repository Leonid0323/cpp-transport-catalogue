#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>



namespace input_reader{

/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
geo::Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2)));

    return {lat, lng};
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

std::pair< std::string_view, std::vector<std::string_view>> ParseStopDescription(std::string_view str) {
    std::pair< std::string_view, std::vector<std::string_view>> parsed_str;
    auto first_comma = str.find(',');
    if (first_comma == str.npos) {
        return parsed_str;
    }
    auto dist_comma = str.find(',', first_comma + 1);
    if (dist_comma == str.npos) {
        parsed_str.first = str;
        return parsed_str;
    }
    parsed_str.first = str.substr(1, dist_comma-1);
    std::string_view str_distance = str.substr(dist_comma + 1);
    parsed_str.second = Split(str_distance, ',');
    return parsed_str;
}

std::pair < std::string_view, int> ParseFromAndDistance(std::string_view str) {
    std::pair < std::string_view, int> to_and_distance;
    auto sym_m = str.find('m');
    to_and_distance.second = std::stod(std::string(str.substr(0, sym_m)));
    to_and_distance.first = str.substr(sym_m + 5);
    return to_and_distance;

}

void InputReader::ApplyCommands([[maybe_unused]] transport_catalogue::TransportCatalogue& catalogue){
    std::vector<size_t> buses;
    std::vector< std::pair< std::string_view, std::vector<std::string_view>>> from_stop_to_stop_distance;
    for (size_t i = 0; i !=commands_.size(); ++i){
        if (commands_[i].command== "Stop"s){
            std::pair< std::string_view, std::vector<std::string_view>> data_description = ParseStopDescription(commands_[i].description);
            catalogue.AddStop(std::move(commands_[i].id), ParseCoordinates(std::string(data_description.first)));
            if (!data_description.second.empty()){
                from_stop_to_stop_distance.push_back({ commands_[i].id, data_description.second });
            }
        } else{
            buses.push_back(i);
        }
    }
    for (const auto& [from, all_to_and_distance] : from_stop_to_stop_distance) {
        for (const auto& to_dist : all_to_and_distance) {
            std::pair < std::string_view, int> to_and_distance = ParseFromAndDistance(to_dist);
            catalogue.AddDistanceStops(from, to_and_distance.first, to_and_distance.second);
        }
    }
    for (size_t i : buses){
        catalogue.AddBus(std::move(commands_[i].id), ParseRoute(commands_[i].description));
    }
}
    
void InputData(transport_catalogue::TransportCatalogue& catalogue, std::istream& input){
    int base_request_count;
    input >> base_request_count >> std::ws;

    {
        InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            std::string line;
            std::getline(input, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }
}
    
}
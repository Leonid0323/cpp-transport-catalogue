#include "json.h"

namespace json {

using namespace std::literals;
    
bool Node::IsInt() const { 
    return std::holds_alternative<int>(*this); 
}    

bool Node::IsPureDouble() const { 
    return std::holds_alternative<double>(*this); 
}
 
bool Node::IsDouble() const { 
    return IsInt() || IsPureDouble(); 
}
   
bool Node::IsBool() const { 
    return std::holds_alternative<bool>(*this); 
}

bool Node::IsString() const { 
    return std::holds_alternative<std::string>(*this); 
}

bool Node::IsNull() const { 
    return std::holds_alternative<std::nullptr_t>(*this); 
}

bool Node::IsArray() const { 
    return std::holds_alternative<Array>(*this); 
}

bool Node::IsMap() const { 
    return std::holds_alternative<Dict>(*this); 
}

int Node::AsInt() const {
    if(!IsInt()){
        throw std::logic_error("");
    }
    return get<int>(*this);
}
    
bool Node::AsBool() const {
    if(!IsBool()){
        throw std::logic_error("");
    }
    return get<bool>(*this);
}
    
double Node::AsDouble() const {
    if(!IsDouble()){
        throw std::logic_error("");
    } else if(IsInt()){
        return get<int>(*this);
    }
    return get<double>(*this);
}

const std::string& Node::AsString() const {
    if(!IsString()){
        throw std::logic_error("");
    }
    return get<std::string>(*this);
}
    
const Array& Node::AsArray() const {
    if(!IsArray()){
        throw std::logic_error("");
    }
    return get<Array>(*this);
}

const Dict& Node::AsMap() const {
    if(!IsMap()){
        throw std::logic_error("");
    }
    return get<Dict>(*this);
}
    
Node LoadNode(std::istream& input);
    
using Number = std::variant<int, double>;

Node LoadNumber(std::istream& input) {
    std::string parsed_num;

    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }

    if (input.peek() == '0') {
        read_char();
    } else {
        read_digits();
    }

    bool is_int = true;

    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            try {
                return Node(std::stoi(parsed_num));
            } catch (...) {
            }
        }
        return Node(std::stod(parsed_num));
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadString(std::istream& input) {
    
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            ++it;
            break;
        } else if (ch == '\\') {
            ++it;
            if (it == end) {
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            throw ParsingError("Unexpected end of line"s);
        } else {
            s.push_back(ch);
        }
        ++it;
    }
    return s;
}

Node LoadArray(std::istream& input) {
    Array result;
    if (input.peek() == -1) {
        throw ParsingError("Array parsing error");
    }
    for (char c; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    return Node(move(result));
}
    
Node LoadNull(std::istream& input) {
    static const std::string nameNull = "null";
    for (size_t i = 0; i < nameNull.size(); i++) {
        if (nameNull.at(i) == input.peek()){
            input.get();
        }
        else {
            throw ParsingError("Null parsing error");
        }
    }
    if (std::isalpha(input.peek())){
        throw ParsingError("Null parsing error");
    }
    return {};
}
    
Node LoadBool(std::istream& input) {
    static const std::string nameFalse = "false"s;
    static const std::string nameTrue = "true"s;
    char c = input.get();
    bool value = (c == 't');
    std::string name = value ? nameTrue : nameFalse;
    for (size_t i = 1; i < name.size(); i++) {
        if (name[i] == input.peek()){
            input.get();
        }
        else throw ParsingError("Bool parsing error");
    }
    if (std::isalpha(input.peek())){
        throw ParsingError("Bool parsing error");
    }
    return Node(value);
}

Node LoadDict(std::istream& input) {
    Dict result;
    if (input.peek() == -1) throw ParsingError("Dict parsing error");
    for (char c; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }
        std::string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }

    return Node(move(result));
}

Node LoadNode(std::istream& input) {
    char c;
    input >> c;
    if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    }
    else if (c == '"') {
        return LoadString(input);
    }
    else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    }
    else if (c == '[') {
        return LoadArray(input);
    }
    else if (c == '{') {
        return LoadDict(input);
    }
    else {
        input.putback(c);
        return LoadNumber(input);
    } 
}

Document::Document(Node root)
    : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(std::istream& input) {
    return Document{LoadNode(input)};
}

void PrintValue(std::ostream& out, int value) {
    out << value;
}
    
void PrintValue(std::ostream& out, double value) {
    out << value;
}
    
void PrintValue(std::ostream& out, std::nullptr_t) {
    out << "null"sv;
}
    
void PrintValue(std::ostream& out, bool value) {
    out << std::boolalpha << value;
}
    
void PrintValue(std::ostream& out, const std::string& str) {
    out << "\""sv;
    for ( char c: str) {
        switch (c){
            case'\\': 
                out << "\\\\"sv;
                break;
            case'"':
                out << "\\\""sv;
                break;
            case'\n':
                out << "\\n"sv;
                break;
            case'\r':
                out << "\\r"sv;
                break;
            case'\t':
                out << "\\t"sv;
                break;
            default:
                out << c;
                break;
        }
    }
    out << "\""sv;
}
    
void PrintValue(std::ostream& out, const Array& arr) {
    out << "["sv;
    bool is_first = true;
    for (size_t i = 0; i < arr.size(); ++i) {
        if(!is_first){
            out << ",";
        }
        PrintNode(out, arr[i]);
        is_first = false;
    }
    out << "]"sv;
}
    
void PrintValue(std::ostream& out, const Dict& dict) {
    out << "{"sv;
    bool is_first = true;
    for (const auto& i : dict) {
        if(!is_first){
            out << ",";
        }
        PrintNode(out, i.first);
        out <<":";
        PrintNode(out,i.second);
        is_first = false;
    }
    out << "}"sv;
}

void PrintNode(std::ostream& out, const Node& node) {
    std::visit(
        [&out](const auto& value){ PrintValue(out, value); },
        node.GetValue());
}
    
void Print(const Document& doc, std::ostream& output) {
    PrintNode(output, doc.GetRoot());
}

}
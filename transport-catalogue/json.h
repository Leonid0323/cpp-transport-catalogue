#pragma once

#include <cassert>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <variant>
#include <vector>


namespace json {

class Node;

using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;
using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node final: private Value{
public:
    using variant::variant;
    
    const Value& GetValue() const { return *this; }
    
    bool IsInt() const;
    bool IsPureDouble() const;
    bool IsDouble() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;
    
    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;
    
    bool operator ==(const Node& rhs) const {
        return *this == rhs.GetValue();
    }
    
    bool operator !=(const Node& rhs) const {
        return !(*this == rhs);
    }
};
    
void PrintValue(std::ostream& out, std::nullptr_t);
void PrintValue(std::ostream& out, bool value);
void PrintValue(std::ostream& out, const std::string& str);
void PrintValue(std::ostream& out, const Array& arr);
void PrintValue(std::ostream& out, const Dict& dict);
void PrintValue(std::ostream& out, int value);
void PrintValue(std::ostream& out, double value);   
void PrintNode(std::ostream& out, const Node& node);

    
class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;
    
    bool operator ==(const Document& rhs) {
        return this->GetRoot() == rhs.GetRoot();
    }
    
    bool operator!=(const Document& rhs) {
        return !(*this == rhs);
    }
private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}
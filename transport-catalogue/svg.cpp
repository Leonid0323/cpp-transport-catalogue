#include "svg.h"

namespace svg {

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, StrokeLineCap cap) {
    switch (cap) {
        case StrokeLineCap::BUTT:
            output << "butt"s;
            break;
        case StrokeLineCap::ROUND:
            output << "round"s;
            break;
        case StrokeLineCap::SQUARE:
            output << "square"s;
            break;
    }
    return output;
}

std::ostream& operator<<(std::ostream& output, StrokeLineJoin join) {
    switch (join) {
        case StrokeLineJoin::ARCS:
            output << "arcs"s;
            break;
        case StrokeLineJoin::BEVEL:
            output << "bevel"s;
            break;
        case StrokeLineJoin::MITER:
            output << "miter"s;
            break;
        case StrokeLineJoin::MITER_CLIP:
            output << "miter-clip"s;
            break;
        case StrokeLineJoin::ROUND:
            output << "round"s;
            break;
    }
    return output;
}
    
void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    RenderObject(context);

    context.out << std::endl;
}

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}
    
void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool is_first = true;
    for (const auto& point : points_){
        if(!is_first){
            out << " "sv;
        }
        out << point.x << ","sv << point.y;
        is_first = false;
    }
    out << "\""sv;
    RenderAttrs(out);
    out <<  "/>"sv;
}

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}
    
Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}
    
Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;
    return *this;
}
    
Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;
    return *this;
}
    
Text& Text::SetData(std::string data) {
    data_ = std::regex_replace(data, std::regex("&"s), "&amp;"s);
    data_ = std::regex_replace(data_, std::regex("\""s), "&quot;"s);
    data_ = std::regex_replace(data_, std::regex("\'"s), "&apos;"s);
    data_ = std::regex_replace(data_, std::regex("<"s), "&lt;"s);
    data_ = std::regex_replace(data_, std::regex(">"s), "&gt;"s);
    return *this;
}
    
void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"sv;
    RenderAttrs(out);
    out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\"";
    out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\"";
    out << " font-size=\""sv << font_size_ << "\"";
    if (!font_family_.empty()){
        out << " font-family=\""sv << font_family_ << "\"";
    }
    if (!font_weight_.empty()){
        out << " font-weight=\""sv << font_weight_ << "\"";
    }
    out << ">"sv;
    out << data_;
    out << "</text>"sv;
}
    
void Document::AddPtr(std::unique_ptr<Object>&& obj){
    objects_.emplace_back(std::move(obj));
}
    
void Document::Render(std::ostream& out) const{
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    for (auto& object : objects_){
        out << "  "sv;
        object->Render(out);
    }
    out << "</svg>"sv;
}
}

namespace shapes{
    
void Triangle::Draw(svg::ObjectContainer& container) const {
    container.Add(svg::Polyline().AddPoint(p1_).AddPoint(p2_).AddPoint(p3_).AddPoint(p1_));
}

svg::Polyline CreateStar(svg::Point center, double outer_rad, double inner_rad, int num_rays) {
    using namespace svg;
    Polyline polyline;
    for (int i = 0; i <= num_rays; ++i) {
        double angle = 2 * M_PI * (i % num_rays) / num_rays;
        polyline.AddPoint({center.x + outer_rad * sin(angle), center.y - outer_rad * cos(angle)});
        if (i == num_rays) {
            break;
        }
        angle += M_PI / num_rays;
        polyline.AddPoint({center.x + inner_rad * sin(angle), center.y - inner_rad * cos(angle)});
    }
    return polyline;
}
    
void Star::Draw(svg::ObjectContainer& container) const {
    container.Add(CreateStar(center_, outer_radius_, inner_radius_, num_rays_).SetFillColor("red").SetStrokeColor("black"));
}  

void Snowman::Draw(svg::ObjectContainer& container) const {
    container.Add(svg::Circle().SetCenter({head_center_.x,head_center_.y + 5*head_radius_}).SetRadius(2*head_radius_).SetFillColor("rgb(240,240,240)").SetStrokeColor("black"));
    container.Add(svg::Circle().SetCenter({head_center_.x,head_center_.y + 2*head_radius_}).SetRadius(1.5*head_radius_).SetFillColor("rgb(240,240,240)").SetStrokeColor("black"));
    container.Add(svg::Circle().SetCenter(head_center_).SetRadius(head_radius_).SetFillColor("rgb(240,240,240)").SetStrokeColor("black"));
}
    
}
#pragma once
#pragma once

#define _USE_MATH_DEFINES 
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include<optional>
#include <regex>
#include <string>
#include <variant>
#include <vector>

namespace svg {

struct Rgb {
    uint16_t red = 0;
    uint16_t green = 0;
    uint16_t blue = 0;
};

struct Rgba {
    uint16_t red = 0;
    uint16_t green = 0;
    uint16_t blue = 0;
    double opacity = 1.0;
};
    
using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

inline const Color NoneColor{"none"};
    
enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

struct OstreamColorPrinter {
    
    std::ostream& out;

    void operator()(std::monostate) const {
        out << "none";
    }
    void operator()(const std::string color) const {
        out << color;
    }
    void operator()(const Rgb color) const {
        out << "rgb(" << color.red << "," << color.green << "," << color.blue << ")";
    }
    void operator()(const Rgba color) const {
        out << "rgba(" << color.red << "," << color.green << "," << color.blue << "," << color.opacity << ")";
    }
};
    
std::ostream& operator<<(std::ostream& output, StrokeLineCap cap);
    
std::ostream& operator<<(std::ostream& output, StrokeLineJoin join);

inline std::ostream& operator<<(std::ostream& output, const Color& color) {
    std::ostringstream strm;
    std::visit(OstreamColorPrinter{strm}, color);
    output << strm.str();
    return output;
}
    
template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    
    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }
    
    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_line_cap_ = line_cap;
        return AsOwner();
    }
    
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_line_join_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const{
        using namespace std::literals;
        if (fill_color_) {
            out << " fill=\""sv << *fill_color_ << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv << *stroke_color_ << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if (stroke_line_cap_) {
            out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
        }
        if (stroke_line_join_) {
            out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
        }
    }

private:
    Owner& AsOwner() {
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_line_cap_;
    std::optional<StrokeLineJoin> stroke_line_join_;
};
    
struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class Circle : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

class Polyline : public Object, public PathProps<Polyline>  {
public:
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;
    
    std::vector<Point> points_;
};

class Text : public Object, public PathProps<Text>  {
public:
    Text& SetPosition(Point pos);
    Text& SetOffset(Point offset);
    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(std::string font_family);
    Text& SetFontWeight(std::string font_weight);
    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;
    
    Point pos_ = {0.0, 0.0};
    Point offset_ = {0.0, 0.0};
    uint32_t font_size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};
    
class ObjectContainer {
public:
    template <typename Obj>
    void Add(Obj obj) {
        AddPtr(std::make_unique<Obj>(std::move(obj)));
    }
    
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
    
    virtual ~ObjectContainer() = default;
};

class Drawable{
public:
    virtual void Draw(ObjectContainer& container) const = 0;
    
    virtual ~Drawable() = default;
};
    
class Document : public ObjectContainer {
public:
    void AddPtr(std::unique_ptr<Object>&& obj) override;

    void Render(std::ostream& out) const;
    
private:
    std::vector<std::unique_ptr<Object>> objects_;
};
    
}

namespace shapes {

class Triangle : public svg::Drawable {
public:
    Triangle(svg::Point p1, svg::Point p2, svg::Point p3)
        : p1_(p1)
        , p2_(p2)
        , p3_(p3) {
    }

    void Draw(svg::ObjectContainer& container) const override;

private:
    svg::Point p1_, p2_, p3_;
};

class Star : public svg::Drawable {
public:
    Star(svg::Point center, double outer_radius, double inner_radius, int num_rays)
    : center_(center), outer_radius_(outer_radius), 
      inner_radius_(inner_radius), num_rays_(num_rays) { 
    }
    
    void Draw(svg::ObjectContainer& container) const override;
    
private:
    svg::Point center_;
    double outer_radius_;
    double inner_radius_;
    int num_rays_;
};
    
class Snowman : public svg::Drawable {
public:    
    Snowman(svg::Point head_center, double head_radius)
    : head_center_(head_center), head_radius_(head_radius){
    }
    
    void Draw(svg::ObjectContainer& container) const override;
    
private:
    svg::Point head_center_;
    double head_radius_;
};
    
}// namespace shapes
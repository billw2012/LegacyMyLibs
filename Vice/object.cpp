// standard
#include <unordered_set>

// boost
#include <boost/range/algorithm.hpp>
#include <boost/variant/static_visitor.hpp>

// solution
#include "Utils/unique_vector.h"
#include "Math/vector4.hpp"

// project
#include "boost_python_wrapper.h"
#include "object.h"
#include "xml_utils.h"
#include "logging.h"
#include "component.h"


namespace vice {;

struct ObjectReg
{
	~ObjectReg()
	{
		if(!activeObjects.empty())
		{
			Logging::global().log(Logging::Error) << activeObjects.size() << " Vice objects leaked.";
		}
	}

	std::unordered_set<Object*> activeObjects;
};

static ObjectReg gReg;

std::unordered_map<std::string, Object::property_variant> Object::_defaultProperties {
	{
		{ std::string("order"), Object::property_variant(int(0)) },
		{ std::string("parent"), Object::property_variant(std::string()) },
		{ std::string("font"), Object::property_variant(std::string()) },
		{ std::string("horiz_align"), Object::property_variant(gltext::Font::HAlignment::AlignLeft) },
		{ std::string("vert_align"), Object::property_variant(gltext::Font::VAlignment::AlignTop) },
		{ std::string("text_color"), Object::property_variant(Color()) },
		{ std::string("text_size"), Object::property_variant(int(0)) },
		{ std::string("x"), Object::property_variant(float(0)) },
		{ std::string("y"), Object::property_variant(float(0)) },
		{ std::string("width"), Object::property_variant(float(0)) },
		{ std::string("height"), Object::property_variant(float(0)) },
		{ std::string("layout_left"), Object::property_variant(Object::PinType::PinMin) },
		{ std::string("layout_right"), Object::property_variant(Object::PinType::PinMin) },
		{ std::string("layout_top"), Object::property_variant(Object::PinType::PinMin) },
		{ std::string("layout_bottom"), Object::property_variant(Object::PinType::PinMin) },
		{ std::string("text"), Object::property_variant(std::string()) },
		{ std::string("text_size"), Object::property_variant(int(12)) },
		//{ std::string("color"), Object::property_variant(Color()) },
		{ std::string("font"), Object::property_variant(std::string("arial")) },
		{ std::string("text_color"), Object::property_variant(Color()) },
		{ std::string("shader"), Object::property_variant(std::string("element_shader")) },
	}
};

Object::Object(const std::string& name /*= std::string()*/, type type_ /*= ObjectType*/) 
	: _name(name)
	, _owner(nullptr)
	, _type(type_)
	, _highlighted(false)
	, _selected(false)
	, _actualLeft(0)
	, _actualRight(0)
	, _actualTop(0)
	, _actualBottom(0)
	//, _parentWidth(0)
	//, _parentHeight(0)
{
	gReg.activeObjects.insert(this);
}

Object::~Object()
{
	// don't do this, if this is last ref then we ALREADY are being removed from the last child list...
	//set_owner(nullptr);
	gReg.activeObjects.erase(this);
}

void Object::set_name( const std::string& name )
{
	_name = name;
}

std::string Object::get_name() const
{
	return _name;
}

void Object::set_parent(const std::string& str)
{
	set_property("parent", str);
}

int Object::get_order() const
{
	return get_property_final_typed<int>("order");
}

void Object::set_order(int order)
{
	set_property("order", order);
}

std::string Object::get_parent() const
{
	return get_property_final_typed<std::string>("parent");
}

void Object::set_position( float x, float y )
{
	set_x(x);
	set_y(y);
}

void Object::set_x(float x)
{
	set_property("x", x);
}

void Object::set_y(float y)
{
	set_property("y", y);
}

void Object::set_x_layout(float x)
{
	set_layout_property("x", x);
}

void Object::set_y_layout(float y)
{
	set_layout_property("y", y);
}

float Object::get_x() const
{
	return get_property_final_typed<float>("x");
}

float Object::get_y() const
{
	return get_property_final_typed<float>("y");
}

void Object::set_size( float width, float height )
{
	set_width(width);
	set_height(height);
}

void Object::set_width(float width)
{
	set_property("width", width);
}

void Object::set_height(float height)
{
	set_property("height", height);
}

void Object::set_width_layout(float width)
{
	set_layout_property("width", width);
}

void Object::set_height_layout(float height)
{
	set_layout_property("height", height);
}

float Object::get_width() const
{
	return get_property_final_typed<float>("width");
}

float Object::get_height() const
{
	return get_property_final_typed<float>("height");
}


Object::PinType::type Object::get_layout_left() const
{
	return get_property_final_typed<PinType::type>("layout_left");
}

Object::PinType::type Object::get_layout_right() const
{
	return get_property_final_typed<PinType::type>("layout_right");
}

Object::PinType::type Object::get_layout_top() const
{
	return get_property_final_typed<PinType::type>("layout_top");
}

Object::PinType::type Object::get_layout_bottom() const
{
	return get_property_final_typed<PinType::type>("layout_bottom");
}

/*

Layout:
when parent width changes re-layout direct children (their size changes will cause recursive layout).
if child x is scaled:
new child x is new parent width * x ratio

x ratio is child x / parent width when x is changed explicitly (rather than by layout change).

*/
void Object::set_property( const std::string& name, const property_variant& value, bool layout /*= false*/ )
{
	// if we are changing width or height then we need to re-layout children
	if (name == "width" || name == "height")
	{
		_owner->size_changing(this);
	}
	_properties[name] = value;
	if (name == "width" || name == "height")
	{
		_owner->size_changed(this);
	}

	//if ((layout || _actualLeft == 0.0f) && name == "x")
	//{
	//	_parentWidth = get_parent_object()->get_width();
	//	_actualLeft = get_x() / _parentWidth;
	//}
	//else if ((layout || _actualTop == 0.0f) && name == "y")
	//{
	//	_parentHeight = get_parent_object()->get_height();
	//	_actualTop = get_y() / _parentHeight;
	//}
	//else if ((layout || _actualRight == 0.0f) && name == "width")
	//{
	//	_parentWidth = get_parent_object()->get_width();
	//	_actualRight = (get_x() + get_width()) / _parentWidth;
	//}
	//else if ((layout || _actualBottom == 0.0f) && name == "height")
	//{
	//	_parentHeight = get_parent_object()->get_height();
	//	_actualBottom = (get_y() + get_height()) / _parentHeight;
	//}
}

void Object::set_animated_property( const std::string& name, const property_variant& value )
{
	_animatedProperties[name] = value;
}

void Object::set_layout_property(const std::string& name, const property_variant& value)
{
	set_property(name, value, true);
}

std::pair<Object::property_variant, bool> Object::get_property_base(const std::string& name) const
{
	auto val = _properties.find(name);
	if (val != _properties.end())
		return{ val->second, true };
	auto fItr = _defaultProperties.find(name);//default_;
	if (fItr != _defaultProperties.end())
		return{ fItr->second, true };
	return{ property_variant(), false };
}

std::pair<Object::property_variant, bool> Object::get_property_final(const std::string& name) const
{
	//auto layout = _layoutProperties.find(name);
	//if (layout != _layoutProperties.end())
	//	return{ layout->second, true };

	//auto animated = _animatedProperties.find(name);
	//if (animated != _animatedProperties.end())
	//	return{ animated->second, true };

	//auto nonAnimatied = _properties.find(name);
	//if (nonAnimatied != _properties.end())
	//	return{ nonAnimatied->second, true };

	//auto fItr = _defaultProperties.find(name);//default_;
	//if (fItr != _defaultProperties.end())
	//	return{ fItr->second, true };
	return{ property_variant(), false };
}

void Object::register_type()
{
	using namespace boost::python;
	class_<Object/*, boost::noncopyable*/, std::shared_ptr<Object> >("Object", init<std::string>())
		.add_property("name", &Object::get_name, &Object::set_name, "Object name")
		.add_property("parent", &Object::get_parent, &Object::set_parent, "Parent name, containing element")
		.add_property("x", &Object::get_x, &Object::set_x, "Object x coordinate")
		.add_property("y", &Object::get_y, &Object::set_y, "Object y coordinate")
		.add_property("width", &Object::get_width, &Object::set_width, "Object width")
		.add_property("height", &Object::get_height, &Object::set_height, "Object height")
		.add_property("owner", make_function(&Object::get_owner, return_value_policy<reference_existing_object>()), &Object::set_owner, "Owning component")
	;
}

void Object::set_owner(ComponentInstance* other)
{
	auto oldOwner = _owner;
	_owner = other;
	if(_owner == oldOwner)
		return ;
	if(oldOwner)
	{
		auto itr = std::find_if(oldOwner->_children.begin(), oldOwner->_children.end(), 
			[this](const Object::ptr& obj) -> bool {
				return obj.get() == this;
		});

		//ERROR HERE: Object erasure causes shared_ptr to deref for a second time, double delete etc..
		//circular dependency
		if(itr != oldOwner->_children.end()) 
			oldOwner->_children.erase(itr);
		//unique_erase(_parent->_children, shared_from_this());
	}
	if(_owner) 
		unique_push(_owner->_children, shared_from_this());
}

const ComponentInstance* Object::get_owner_const() const
{
	return _owner;
}

ComponentInstance* Object::get_owner()
{
	return _owner;
}

Object* Object::get_parent_object()
{
	return get_owner()->get_object_parent(this);
}

const Object* Object::get_parent_object() const
{
	return get_owner_const()->get_object_parent(const_cast<Object*>(this));
}

bool Object::is_hit_global(float x, float y) const
{
	math::Vector2f localPos = from_global(math::Vector2f(x, y));
	
	return localPos.x >= 0.0f && localPos.x <= get_width() &&
		localPos.y >= 0.0f && localPos.y <= get_height();
}

math::Vector2f Object::from_global(const math::Vector2f& pt) const
{
	return math::Vector2f(globalTransform.inverse() * math::Vector4f(pt, 0.0f, 1.0f));
}

math::Vector2f Object::from_component(const math::Vector2f& pt) const
{
	return math::Vector2f(componentTransform.inverse() * math::Vector4f(pt, 0.0f, 1.0f));
}

math::Vector2f Object::to_global(const math::Vector2f& pt) const
{
	return math::Vector2f(globalTransform * math::Vector4f(pt, 0.0f, 1.0f));
}

math::Vector2f Object::to_component(const math::Vector2f& pt) const
{
	return math::Vector2f(componentTransform * math::Vector4f(pt, 0.0f, 1.0f));
}

void Object::clear_animated_properties()
{
	_animatedProperties.clear();
}

void Object::clear_layout_properties()
{
	//_layoutProperties.clear();
}

void Color::register_type()
{
	using namespace boost::python;
	class_<Color>("Color")
		.def_readwrite("r", &Color::r)
		.def_readwrite("g", &Color::g)
		.def_readwrite("b", &Color::b)
		.def_readwrite("a", &Color::a);
}

struct ToStringVisitor : public boost::static_visitor < >
{
	ToStringVisitor(Object::StringType& strType) : _strType(strType) {}

	void operator()(const std::string& val) const
	{
		_strType = Object::StringType(val, Object::PropType::String);
	}

	void operator()(int val) const
	{
		std::stringstream ss;
		ss << val;
		_strType = Object::StringType(ss.str(), Object::PropType::Int);
	}


	void operator()(float val) const
	{
		std::stringstream ss;
		ss << val;
		_strType = Object::StringType(ss.str(), Object::PropType::Float);
	}


	void operator()(const Color& val) const
	{
		std::stringstream ss;
		ss << val.r << " " << val.g << " " << val.b << " " << val.a;
		_strType = Object::StringType(ss.str(), Object::PropType::Color);
	}

	void operator()(Object::PinType::type val) const
	{
		std::stringstream ss;
		ss << val;
		_strType = Object::StringType(ss.str(), Object::PropType::Layout);
	}

	void operator()(gltext::Font::HAlignment val) const
	{
		std::stringstream ss;
		ss << val;
		_strType = Object::StringType(ss.str(), Object::PropType::HAlignment);
	}

	void operator()(gltext::Font::VAlignment val) const
	{
		std::stringstream ss;
		ss << val;
		_strType = Object::StringType(ss.str(), Object::PropType::VAlignment);
	}


private:
	Object::StringType& _strType;
};

Object::StringType Object::property_to_string(const property_variant& prop)
{
	StringType str;
	boost::apply_visitor(ToStringVisitor(str), prop);
	return str;
}


Object::property_variant Object::string_to_property(const StringType& strType)
{
	switch (strType.type)
	{
	case PropType::Int:
		return parse_val<int>(strType.val);
	case PropType::Float:
		return parse_val<float>(strType.val);
	case PropType::Color:
		return parse_val<Color>(strType.val);
	case PropType::Layout:
		return parse_val<Object::PinType::type>(strType.val);
	case PropType::HAlignment:
		return parse_val<gltext::Font::HAlignment>(strType.val);
	case PropType::VAlignment:
		return parse_val<gltext::Font::VAlignment>(strType.val);
	default:
		return strType.val;
	};
}

boost::filesystem::path Object::get_load_context() const
{
	return _owner->get_load_path().parent_path();
}

void Object::parent_size_changing(Object* parent)
{
	//_parentWidth = parent->get_width();
	//_parentHeight = parent->get_height();
	//auto x = get_x();
	//auto y = get_x();
	//_rightRatio = (x + get_width()) / _parentWidth;
	//_topRatio = y / _parentHeight;
	//_bottomRatio = (y + get_height()) / _parentHeight;
}

void Object::parent_size_changed(Object* parent)
{
	//auto newParentWidth = parent->get_width();
	//auto newParentHeight = parent->get_height();

	//auto layoutLeft = get_layout_left(),
	//	layoutRight = get_layout_right(),
	//	layoutTop = get_layout_top(),
	//	layoutBottom = get_layout_bottom();

	//if (layoutLeft == PinType::Stretch)
	//	set_property("x", newParentWidth * _actualLeft, true);
	//else if (layoutLeft == PinType::PinMax)
	//	set_property("x", newParentWidth - (_parentWidth - (get_x() + get_width())), true);

	//if (layoutRight == PinType::Stretch)
	//	set_property("width", newParentWidth * _actualRight, true);
	//else if (layoutRight == PinType::PinMax)
	//	set_property("width", newParentWidth * _actualRight, true);

	//if (layoutTop == PinType::Stretch)
	//	set_property("y", newParentHeight * _actualTop, true);
	//if (layoutBottom == PinType::Stretch)
	//	set_property("height", newParentHeight * _actualBottom, true);
}

float Object::get_left() const
{
	return get_x();
}

float Object::get_right() const
{
	return get_x() + get_width();
}

float Object::get_top() const
{
	return get_y();
}

float Object::get_bottom() const
{
	return get_y() + get_height();
}

void Object::set_left(float left)
{
	set_x(left);
}

void Object::set_right(float right)
{
	set_width(right - get_x());
}

void Object::set_top(float top)
{
	set_y(top);
}

void Object::set_bottom(float bottom)
{
	set_height(bottom - get_y());
}

void Object::update_actual_edges()
{
	auto parent = get_parent_object();
	auto parentWidth = parent->get_width();
	auto parentHeight = parent->get_height();

	auto layoutLeft = get_layout_left(),
		layoutRight = get_layout_right(),
		layoutTop = get_layout_top(),
		layoutBottom = get_layout_bottom();

	switch (get_layout_left())
	{
	case PinType::PinMin:	_actualLeft = get_left(); break;
	case PinType::PinMax:	_actualLeft = parentWidth - get_left(); break;
	case PinType::Stretch:	_actualLeft = get_left() / parentWidth; break;
	};
	switch (get_layout_right())
	{
	case PinType::PinMin:	_actualRight = get_right(); break;
	case PinType::PinMax:	_actualRight = parentWidth - get_right(); break;
	case PinType::Stretch:	_actualRight = get_right() / parentWidth; break;
	};
	switch (get_layout_top())
	{
	case PinType::PinMin:	_actualTop = get_top(); break;
	case PinType::PinMax:	_actualTop = parentHeight - get_top(); break;
	case PinType::Stretch:	_actualTop = get_top() / parentHeight; break;
	};
	switch (get_layout_bottom())
	{
	case PinType::PinMin:	_actualBottom = get_bottom(); break;
	case PinType::PinMax:	_actualBottom = parentHeight - get_bottom(); break;
	case PinType::Stretch:	_actualBottom = get_bottom() / parentHeight; break;
	};
}

std::string Object::PropType::to_string(type val)
{
	switch (val)
	{
	case PropType::Int:			return "int";
	case PropType::Float:		return "float";
	case PropType::Color:		return "color";
	case PropType::Layout:		return "layout";
	case PropType::HAlignment:	return "halignment";
	case PropType::VAlignment:	return "valignment";
	default:					return "string";
	};
}

vice::Object::PropType::type Object::PropType::from_string(const std::string& str)
{
	if (str == "int")			return PropType::Int;
	if (str == "float")			return PropType::Float;
	if (str == "color")			return PropType::Color;
	if (str == "layout")		return PropType::Layout;
	if (str == "halignment")	return PropType::HAlignment;
	if (str == "valignment")	return PropType::VAlignment;
	return String;
}

Object::EditProps Object::get_edit_properties()
{
	EditProps props;
	add_edit_defaults(props, { "parent", "order", "x", "y", "width", 
		"height", "layout_left", "layout_right", "layout_top", "layout_bottom" });

	for (const auto& prop : _properties)
	{
		set_edit_value(props, prop.first, prop.second);
	}
	return std::move(props);
}

void Object::add_edit_default(EditProps& cont, const std::string& name)
{
	cont.push_back({ name, _defaultProperties[name] });
}

void Object::set_edit_value(EditProps& cont, const std::string& name, const property_variant& value)
{
	auto fItr = boost::range::find_if(cont, [&](EditProps::value_type& prop) -> bool {
		return prop.name == name;
	});

	if (fItr != cont.end())
		fItr->value = value;
	else
		cont.push_back({ name, value });
}

void Object::add_edit_defaults(EditProps& cont, const std::vector<std::string>& names)
{
	for (const auto& name : names)
		cont.push_back({ name, _defaultProperties[name] });
	//cont[name] = _defaultProperties[name];
}

}
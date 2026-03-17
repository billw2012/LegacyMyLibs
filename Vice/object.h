#ifndef object_h__
#define object_h__

// standard
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

// boost
#include <boost/variant.hpp>

// solution
#include "tinyxml.h"
#include "Math/matrix4.hpp"
#include "Math/vector2.h"

// project
#include "boost_python_wrapper.h"
#include "xml_utils.h"
#include "gltext/gltext.h"

namespace vice {;

struct Color
{
	Color(float r_ = 1, float g_ = 1, float b_ = 1, float a_ = 1) : r(r_), g(g_), b(b_), a(a_) {}

	static void register_type();

	float r, g, b, a;
};

template <>
inline Color parse_val<Color>(const std::string& str)
{
	std::stringstream ss(str);
	Color c;
	ss >> c.r >> c.g >> c.b >> c.a;
	return c;
}

template < class Ty_ >
struct python_vice_ptr
{
	typedef Ty_ value_type;
	typedef value_type* ptr_type;
	typedef std::shared_ptr<value_type> shared_ptr_type;
	typedef python_vice_ptr<value_type> this_type;

	template < class OtherTy_ >
	friend struct python_vice_ptr;

	python_vice_ptr() : _viceObj(nullptr) {}

	explicit python_vice_ptr(ptr_type viceObj) : _viceObj(viceObj) {}

	python_vice_ptr(const shared_ptr_type& viceObj, const boost::python::object& pythonObj = boost::python::object())
		: _viceObj(viceObj)
		, _pythonObj(pythonObj) {}

	template < class OtherTy_ >
	python_vice_ptr(const python_vice_ptr<OtherTy_>& other)
		: _viceObj(other._viceObj)
		, _pythonObj(other._pythonObj) {}

	ptr_type operator->() const
	{
		return _viceObj.get();
	}

	value_type& operator*() const
	{
		return *_viceObj;
	}

	operator bool() const 
	{
		return _viceObj != nullptr;
	}

	bool operator==(const this_type& other) const
	{
		return _viceObj == other._viceObj;
	}

	ptr_type get() const 
	{
		return _viceObj.get();
	}

	const shared_ptr_type& ptr() const 
	{
		return _viceObj;
	}

private:
	shared_ptr_type _viceObj;
	boost::python::object _pythonObj;
};

struct ComponentInstance;

struct Object : public std::enable_shared_from_this<Object>
{
	friend struct ComponentInstance;
	friend struct ComponentLibrary;

	enum type 
	{
		ObjectType,
		ElementType,
		ComponentType
	};

	// layout style
	struct PinType { enum type {
		PinMin,
		PinMax,
		Stretch
	};};

	typedef python_vice_ptr<Object> ptr;

	typedef boost::variant < 
		std::string, 
		int, 
		float, 
		Color, 
		PinType::type, 
		gltext::Font::HAlignment,
		gltext::Font::VAlignment
	> property_variant;

	typedef std::unordered_map<std::string, property_variant> PropertyMap;

	Object(const std::string& name = std::string(), type type_ = ObjectType);
	~Object();

	type get_type() const { return _type; }

	void set_name(const std::string& name);
	std::string get_name() const;

	void set_parent(const std::string& str);
	std::string get_parent() const;

	int get_order() const;
	void set_order(int order);

	void set_position(float x, float y);
	void set_x(float x);
	void set_y(float y);
	void set_x_layout(float x);
	void set_y_layout(float y);
	float get_x() const;
	float get_y() const;

	void set_size(float width, float height);
	void set_width(float width);
	void set_height(float height);
	void set_width_layout(float width);
	void set_height_layout(float height);
	float get_width() const;
	float get_height() const;

	PinType::type get_layout_left() const;
	PinType::type get_layout_right() const;
	PinType::type get_layout_top() const;
	PinType::type get_layout_bottom() const;

	void set_rotation(float rotation);
	float get_rotation() const;

	void set_pivot(float pivotX, float pivotY);
	float get_pivot_x() const;
	float get_pivot_y() const;

	virtual void set_property(const std::string& name, const property_variant& value, bool layout = false);
	virtual void set_animated_property(const std::string& name, const property_variant& value);
	void set_layout_property(const std::string& name, const property_variant& value);

	std::pair<property_variant, bool> get_property_final(const std::string& name) const;
	std::pair<property_variant, bool> get_property_base(const std::string& name) const;

	void set_owner(ComponentInstance* other);
	const ComponentInstance* get_owner_const() const;
	ComponentInstance* get_owner();

	Object* get_parent_object();
	const Object* get_parent_object() const;

	const math::Matrix4f& get_transform() const { return transform; }
	const math::Matrix4f& get_global_transform() const { return globalTransform; }
	const math::Matrix4f& get_component_transform() const { return componentTransform; }

	bool is_hit_global(float x, float y) const;
	math::Vector2f from_global(const math::Vector2f& pt) const;
	math::Vector2f from_component(const math::Vector2f& pt) const;
	math::Vector2f to_global(const math::Vector2f& pt) const;
	math::Vector2f to_component(const math::Vector2f& pt) const;

	// design mode functions >>>
	void set_selected(bool selected) { _selected = selected; }
	bool is_selected() const { return _selected; }
	void set_highlighted(bool highlighted) { _highlighted = highlighted; }
	bool is_highlighted() const { return _highlighted; }

	struct PropPair
	{
		std::string name;
		property_variant value;
	};
	using EditProps = std::vector < PropPair > ;

	virtual EditProps get_edit_properties();
	// << design mode functions

	const PropertyMap& get_properties() const { return _properties; }
	boost::filesystem::path get_load_context() const;


	void parent_size_changing(Object* parent);
	void parent_size_changed(Object* parent);

	static void register_type();

	struct PropType {
		enum type {
			String, Int, Float, Color, Layout, HAlignment, VAlignment
		};
		static std::string to_string(type val);
		static type from_string(const std::string& str);
	};

	struct StringType
	{
		StringType() {}
		StringType(const std::string& val_, PropType::type type_)
			: val(val_), type(type_) {}

		std::string val;
		PropType::type type;
	};

	static StringType property_to_string(const property_variant& prop);
	static property_variant string_to_property(const StringType& strType);

private:
	float get_left() const;
	float get_right() const;
	float get_top() const;
	float get_bottom() const;

	void set_left(float left);
	void set_right(float right);
	void set_top(float top);
	void set_bottom(float bottom);

	void update_actual_edges();

protected:
	virtual void clear_animated_properties();
	virtual void clear_layout_properties();

	template < class Ty_ >
	Ty_ get_property_final_typed(const std::string& name) const
	{
		return boost::get<Ty_>(get_property_final(name).first);
	}

	template < class Ty_, class Cont_ >
	static Ty_ get_property_or_default(const Cont_& cont, const std::string& name)
	{
		auto val = cont.find(name);
		if (val != cont.end())
			return boost::get<Ty_>(val->second);
		return get_default<Ty_>(name);
	}

	template < class Ty_ >
	static Ty_ get_default(const std::string& name)
	{
		return boost::get<Ty_>(_defaultProperties[name]);
	}

	static void add_edit_default(EditProps& cont, const std::string& name);
	static void set_edit_value(EditProps& cont, const std::string& name, const property_variant& value);
	static void add_edit_defaults(EditProps& cont, const std::vector<std::string>& names);

	static std::unordered_map<std::string, Object::property_variant> _defaultProperties;

private:
	PropertyMap _properties, _animatedProperties;
	std::string _name;
	ComponentInstance* _owner;

	// transform relative to parent
	math::Matrix4f transform;
	// absolute global transform
	math::Matrix4f globalTransform;
	// transform relative to owner transform
	math::Matrix4f componentTransform;

	// design mode >>
	bool _highlighted, _selected;
	// << design mode

	// these values depend on layout mode for each edge:
	// if it is PinMin then they are the absolute position relative to the minimum parent edge (top left)
	// if it is PinMax then they are the absolute position relative to the maximum parent edge (bottom right)
	// if it is Stretch then they are the absolute position / the parent dimensions (x/width, y/height)
	float _actualLeft, _actualRight, _actualTop, _actualBottom;

	//// if pinning is set to max then old parent width and height are required to determine
	//// new position when they change.
	//float _parentWidth, _parentHeight;

protected:
	type _type;
};

}

namespace std {
	inline std::ostream& operator<< (std::ostream& stream, const vice::Object::PinType::type& val)
	{
		switch (val)
		{
		case vice::Object::PinType::PinMin:	return (stream << "min");
		case vice::Object::PinType::PinMax:	return (stream << "max");
		case vice::Object::PinType::Stretch:
		default:							return (stream << "stretch");
		};
	}

	inline std::istream& operator>> (std::istream& stream, vice::Object::PinType::type& val)
	{
		std::string pinStr;
		stream >> pinStr;
		if (stream.fail()) return stream;
		if (pinStr == "min")
			val = vice::Object::PinType::PinMin;
		else if (pinStr == "max")
			val = vice::Object::PinType::PinMax;
		else if (pinStr == "stretch")
			val = vice::Object::PinType::Stretch;
		else
			stream.setstate(std::ios_base::failbit);

		return stream;
	}
}

namespace std { ;

template < typename Ty_ >
struct hash< vice::python_vice_ptr<Ty_> >
{
	std::size_t operator()(const vice::python_vice_ptr<Ty_>& val) const
	{
		return std::hash<Ty_*>()(val.get());
	}
};

}
#endif // object_h__

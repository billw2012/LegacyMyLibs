
#include <boost/variant/get.hpp>

#include "boost_python_wrapper.h"


#include "element.h"
#include "xml_utils.h"

#include "component.h"

namespace vice {;

Element::Element(const std::string& name /*= std::string()*/) 
	: Object(name, ElementType)
	, _material(new scene::Material())
	, _shaderChanged(true)
{
	_material->set_effect(ComponentLibrary::get_shader(get_default<std::string>("shader") + ".xml"));
}

void Element::set_text( const std::string& text )
{
	set_property("text", text);
}

std::string Element::get_text() const
{
	return get_property_final_typed<std::string>("text");
}

void Element::set_shader(const std::string& shaderName)
{
	set_property("shader", shaderName);
	_shaderChanged = true;
	//auto shader = ComponentLibrary::get_shader(shaderName + ".xml", get_load_context());
	//if (!shader)
	//{
	//	shader = ComponentLibrary::get_shader(get_default<std::string>("shader") + ".xml");
	//}
	//_material->set_effect(shader);
}

std::string Element::get_shader() const
{
	return get_property_final_typed<std::string>("shader");
}

scene::Material::ptr Element::get_material()
{
	if (_shaderChanged)
	{
		auto shaderName = get_shader();
		if (!shaderName.empty())
			_material->set_effect(ComponentLibrary::get_shader(shaderName + ".xml", get_load_context()));
		_shaderChanged = false;
	}

	return _material;
}

void Element::set_font(const std::string& font)
{
	set_property("font", font);
}

std::string Element::get_font() const
{
	return get_property_final_typed<std::string>("font");
}

void Element::set_text_size(int size)
{
	set_property("text_size", size);
}

int Element::get_text_size() const
{
	return get_property_final_typed<int>("text_size");
}

void Element::set_text_color(const Color& color)
{
	set_property("text_color", color);
}

Color Element::get_text_color() const
{
	return get_property_final_typed<Color>("text_color");
}

void Element::set_horiz_align(const gltext::Font::HAlignment& halign)
{
	set_property("horiz_align", halign);
}

gltext::Font::HAlignment Element::get_text_horiz_align() const
{
	return get_property_final_typed<gltext::Font::HAlignment>("horiz_align");
}

void Element::set_vert_align(const gltext::Font::VAlignment& valign)
{
	set_property("vert_align", valign);
}

gltext::Font::VAlignment Element::get_text_vert_align() const
{
	return get_property_final_typed<gltext::Font::VAlignment>("vert_align");
}

//void Element::set_color(const Color& color)
//{
//	set_property("color", color);
//}
//
//Color Element::get_color() const
//{
//	return get_property_final_typed<Color>("color");
//}

void Element::register_type()
{
	using namespace boost::python;
	class_<Element, /*boost::noncopyable,*/ bases<Object>, std::shared_ptr<Element> >("Element", init<std::string>())
		.add_property("text", &Element::get_text, &Element::set_text)
		.add_property("font", &Element::get_font, &Element::set_font)
		.add_property("text_size", &Element::get_text_size, &Element::set_text_size)
		.add_property("text_color", &Element::get_text_color, &Element::set_text_color)
		//.add_property("color", &Element::get_color, &Element::set_color)
		;
	//implicitly_convertible<std::shared_ptr<Element>, std::shared_ptr<Object> >();
}

Object::property_variant get_shader_default(const Object::PropType::type& type)
{
	switch (type)
	{
	case Object::PropType::Float:
		return float(0);
	case Object::PropType::Color:
		return Color();
	case Object::PropType::String:
		return std::string();
	case Object::PropType::Int:
	default:
		return int(0);
	}
}

Object::EditProps Element::get_edit_properties()
{
	EditProps ourProps;
	add_edit_defaults(ourProps, { "text", "text_size", /*"color",*/ "font", "text_color", "horiz_align", "vert_align", "shader" });

	auto shader = get_material()->get_effect();
	if (shader)
	{
		const auto& paramNames = shader->get_parameters();
		for (const auto& nameParamPair : paramNames)
		{
			if (nameParamPair.second.editable)
			{
				ourProps.push_back({ nameParamPair.first, get_shader_default (PropType::from_string(nameParamPair.second.type))});
			}
		}
	}

	EditProps props = Object::get_edit_properties();
	for (const auto& prop : props)
	{
		set_edit_value(ourProps, prop.name, prop.value);
		//ourProps[prop.first] = prop.second;
	}

	return std::move(ourProps);
}

struct PropertyVariantConvert : public boost::static_visitor < >
{
	PropertyVariantConvert(effect::Effect::ParameterVariantType& target, Object* elem) 
		: _target(target)
		, _elem(elem) {}

	void operator()(int val) 
	{
		_target = val;
	}

	void operator()(float val)
	{
		_target = val;
	}

	void operator()(const vice::Color& val) 
	{
		_target = math::Vector4f(val.r, val.g, val.b, val.a);
	}

	void operator()(const std::string& val) 
	{
		_target = ComponentLibrary::get_texture(val, _elem->get_load_context());
	}

	void operator()(vice::Object::PinType::type /*val*/) {}
	void operator()(gltext::Font::HAlignment /*val*/) {}
	void operator()(gltext::Font::VAlignment /*val*/) {}

private:
	effect::Effect::ParameterVariantType& _target;
	Object* _elem;
};

void Element::update_shader_values()
{
	// set shader property if shader is set
	auto shader = _material->get_effect();
	if (shader)
	{
		const auto& paramNames = shader->get_parameters();
		for (const auto& nameParamPair : paramNames)
		{
			auto valExistsPair = get_property_final(nameParamPair.first);
			if (valExistsPair.second)
			{
				effect::Effect::ParameterVariantType var;
				PropertyVariantConvert converter(var, this);
				boost::apply_visitor(converter, valExistsPair.first);
				_material->set_parameter(nameParamPair.first, var);
			}
		}
	}
}

void Element::set_property(const std::string& name, const property_variant& value, bool layout /*= false*/)
{
	Object::set_property(name, value, layout);
	if (name == "shader")
	{
		_shaderChanged = true;
	}
}

}

#ifndef element_h__
#define element_h__

#include "object.h"
#include "tinyxml.h"
#include "gltext/gltext.h"

namespace vice {;


/*
 *	The base element of a vice scene.
 */

//struct TextSettings
//{
//	std::string name;
//	int size;
//	Color color;
//	gltext::Font::Alignment horizAlign, vertAlign;
//
//	friend
//};

struct Element : public Object
{
	typedef python_vice_ptr<Element> ptr;

	Element(const std::string& name = std::string());

	void set_text(const std::string& text);
	std::string get_text() const;

	void set_shader(const std::string& shaderName);
	std::string get_shader() const;

	scene::Material::ptr get_material();

	void set_font(const std::string& font);
	std::string get_font() const;

	void set_text_size(int size);
	int get_text_size() const;

	void set_text_color(const Color& color);
	Color get_text_color() const;

	void set_horiz_align(const gltext::Font::HAlignment& halign);
	gltext::Font::HAlignment get_text_horiz_align() const;

	void set_vert_align(const gltext::Font::VAlignment& valign);
	gltext::Font::VAlignment get_text_vert_align() const;

	virtual void set_property(const std::string& name, const property_variant& value, bool layout = false) override;

// 	void set_color(const Color& color);
// 	Color get_color() const;

	// design mode functions >>>
	virtual EditProps get_edit_properties() override;
	// << design mode functions

	void update_shader_values();

	static void register_type();


private:
	bool _shaderChanged;
	scene::Material::ptr _material;
};

}

#endif // element_h__

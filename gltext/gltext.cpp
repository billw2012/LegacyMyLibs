/*
 * Copyright 2011 Branan Purvine-Riley
 *
 *  This is part of gltext, a text-rendering library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#pragma warning(disable:4996) 

#include <assert.h>
#include <math.h>
#include <map>
#include <array>

#include <boost/range/algorithm/find.hpp>

#include "harfbuzz/hb-ft.h"

#include "GLBase/sdlgl.h"
#include "Utils/on_scope_exit.h"

#include "gltext.h"

#define GLYPH_VERT_SIZE (4*4*sizeof(GLfloat))
#define GLYPH_IDX_SIZE (6*sizeof(GLushort))

struct GlyphVert {
	float x;
	float y;
	float s;
	float t;
};

struct FontSystem {
public:
	static FontSystem& instance() {
		static FontSystem singleton;
		return singleton;
	}

	FontSystem()
	{
		FT_Init_FreeType(&library);
	}

	~FontSystem()
	{
		FT_Done_FreeType(library);
	}

	FT_Library library;

};


namespace gltext {
	;

struct FontPimpl
{
	std::string filename;
	unsigned size;
	FT_Face face;
	hb_font_t* font;

	GLuint vao;
	GLuint vbo;
	GLuint ibo;
	glbase::Texture::ptr tex;

	GLuint texpos_x;
	GLuint texpos_y;
	GLuint num_glyphs_cached;

	unsigned window_w, window_h;

	int y_size;
	int x_size;

	int ascender;
	int descender;
	int lineGap;
	int spaceWidth;
	math::Rectanglei bbox;

	unsigned pen_x, pen_y;

	unsigned cache_w, cache_h;

	math::Vector4f color;

	std::map<FT_UInt, unsigned> glyphs;

	template < class Ty_ >
	int from_font_units_x(Ty_ val)
	{
		return static_cast<int>(ceil(double(val) * double(face->size->metrics.x_ppem) / double(face->units_per_EM)));
	}

	template < class Ty_ >
	int from_font_units_y(Ty_ val)
	{
		return static_cast<int>(ceil(double(val) * double(face->size->metrics.y_ppem) / double(face->units_per_EM)));
	}

	void init()
	{
		FontSystem& system = FontSystem::instance();
		FT_Error error;
		error = FT_New_Face(system.library, filename.c_str(), 0, &face);
		if (error)
		{
			throw FtException();
		}
		error = FT_Set_Pixel_Sizes(face, 0, size);
		if (error)
		{
			FT_Done_Face(face);
			throw FtException();
		}

		font = hb_ft_font_create(face, 0);


		x_size = from_font_units_x(face->max_advance_width); // static_cast<int>(ceil(double(face->max_advance_width) * double(face->size->metrics.x_ppem) / double(face->units_per_EM)));
		y_size = from_font_units_y(face->height); //static_cast<int>(ceil(double(face->height) * double(face->size->metrics.y_ppem) / double(face->units_per_EM)));

		ascender = from_font_units_y(face->ascender); //static_cast<int>(ceil(double(face->ascender) * double(face->size->metrics.y_ppem) / double(face->units_per_EM)));
		descender = from_font_units_y(face->descender); //static_cast<int>(floor(double(face->descender) * double(face->size->metrics.y_ppem) / double(face->units_per_EM)));
		lineGap = y_size - (ascender - descender);

		bbox = math::Rectanglei{ from_font_units_x(face->bbox.xMin), from_font_units_y(face->bbox.yMin), from_font_units_x(face->bbox.xMax), from_font_units_y(face->bbox.yMax) };

		texpos_x = 0;
		texpos_y = 0;
		num_glyphs_cached = 0;

		short max_glyphs = (cache_w / x_size)*(cache_h / y_size);

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ibo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ARRAY_BUFFER, GLYPH_VERT_SIZE*max_glyphs, NULL, GL_DYNAMIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLYPH_IDX_SIZE*max_glyphs, NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(2 * sizeof(float)));

		tex = std::make_shared<glbase::Texture>();
		tex->create2D(GL_TEXTURE_2D, cache_w, cache_h, 1, GL_RED, GL_UNSIGNED_BYTE, NULL, GL_R8);

		hb_buffer_t* buffer = hb_buffer_create();
		on_scope_exit({ hb_buffer_destroy(buffer); });
		hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
		hb_buffer_add_utf8(buffer, " A", 1, 0, 1);
		hb_shape(font, buffer, NULL, 0);
		unsigned len = hb_buffer_get_length(buffer);
		hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(buffer, 0);
		hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, 0);
		spaceWidth = positions[0].x_advance >> 6;
	}

	void cleanup()
	{
		hb_font_destroy(font);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ibo);
		glDeleteVertexArrays(1, &vao);
	}

	std::map<FT_UInt, unsigned>::iterator cacheGlyph(FT_UInt codepoint)
	{
		if (num_glyphs_cached == (cache_w / x_size)*(cache_h / y_size))
			throw CacheOverflowException();
		FT_Error error;
		error = FT_Load_Glyph(face, codepoint, FT_LOAD_RENDER);
		if (error)
			throw FtException();
		if (face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
			throw BadFontFormatException();
		int pitch = face->glyph->bitmap.pitch;
		bool need_inverse_texcoords = true;
		if (pitch < 0)
		{
			pitch = -pitch;
			need_inverse_texcoords = false;
		}
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch);
		if (texpos_x + face->glyph->bitmap.width > cache_w)
		{
			texpos_x = 0;
			texpos_y += y_size;
		}
		glTexSubImage2D(GL_TEXTURE_2D, 0, texpos_x, texpos_y, face->glyph->bitmap.width, face->glyph->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

		float hori_offset = static_cast<float>(face->glyph->bitmap_left);
		float vert_offset = static_cast<float>(face->glyph->bitmap_top) - static_cast<float>(face->glyph->bitmap.rows);

		GlyphVert corners[4];
		GlyphVert& bl = corners[0];
		GlyphVert& ul = corners[1];
		GlyphVert& br = corners[2];
		GlyphVert& ur = corners[3];

		bl.x = 0.0f + hori_offset;
		bl.y = -(0.0f + vert_offset);
		bl.s = static_cast<float>(texpos_x) / static_cast<float>(cache_w);
		if (need_inverse_texcoords)
			bl.t = (static_cast<float>(texpos_y)+static_cast<float>(face->glyph->bitmap.rows)) / float(cache_h);
		else
			bl.t = static_cast<float>(texpos_y) / static_cast<float>(cache_h);

		br.x = face->glyph->bitmap.width + hori_offset;
		br.y = -(0.0f + vert_offset);
		br.s = (static_cast<float>(texpos_x)+static_cast<float>(face->glyph->bitmap.width)) / float(cache_w);
		br.t = bl.t;

		ul.x = 0.0f + hori_offset;
		ul.y = -(static_cast<float>(face->glyph->bitmap.rows) + vert_offset);
		ul.s = bl.s;
		if (need_inverse_texcoords)
			ul.t = static_cast<float>(texpos_y) / static_cast<float>(cache_h);
		else
			ul.t = (static_cast<float>(texpos_y)+static_cast<float>(face->glyph->bitmap.rows)) / static_cast<float>(cache_h);

		ur.x = br.x;
		ur.y = ul.y;
		ur.s = br.s;
		ur.t = ul.t;

		unsigned short glyph_offset = num_glyphs_cached * 4;
		unsigned short indices[6] = { glyph_offset + 0U, glyph_offset + 2U, glyph_offset + 3U, glyph_offset + 0U, glyph_offset + 3U, glyph_offset + 1U };
		glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)(num_glyphs_cached*GLYPH_VERT_SIZE), GLYPH_VERT_SIZE, corners);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (GLintptr)(num_glyphs_cached*GLYPH_IDX_SIZE), GLYPH_IDX_SIZE, indices);
		CHECK_OPENGL_ERRORS;
		texpos_x += x_size;
		num_glyphs_cached++;
		return glyphs.insert(std::make_pair(codepoint, num_glyphs_cached - 1)).first;
	}
};

Font::Font(std::string font_file, unsigned size, unsigned cache_w, unsigned cache_h)
{
	self = new FontPimpl;
	self->filename = font_file;
	self->size = size;
	self->cache_w = cache_w;
	self->cache_h = cache_h;

	// These are initialized here so that they work correctly when a font is de-inited and re-inited
	self->pen_x = 0;
	self->pen_y = 0;
	self->color = math::Vector4f::One;
	try
	{
		self->init();
	}
	catch (Exception&)
	{
		delete self;
		self = 0;
		throw;
	}
}

Font::Font() : self(0) {}

Font::~Font()
{
	if (self) {
		self->cleanup();
		delete self;
	}
}

void Font::setPenColor(const math::Vector4f& color_)
{
	if (!self)
		throw EmptyFontException();
	self->color = color_;
}

// void Font::setPointSize(unsigned int size) 
// {
// 	// TODO: implement this in a slightly more performant fashion
// 	self->cleanup();
// 	self->size = size;
// 	self->glyphs.clear();
// 	self->init();
// }

void Font::cacheCharacters(const std::string& chars)
{
	if (!self)
		throw EmptyFontException();
	hb_buffer_t* buffer = hb_buffer_create();
	on_scope_exit({ hb_buffer_destroy(buffer); });
	hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
	hb_buffer_add_utf8(buffer, chars.c_str(), static_cast<int>(chars.size()), 0, static_cast<int>(chars.size()));
	hb_shape(self->font, buffer, NULL, 0);

	unsigned len = hb_buffer_get_length(buffer);
	hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(buffer, 0);
	hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, 0);

	self->tex->bind();

	glBindVertexArray(self->vao);
	glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (unsigned i = 0; i < len; i++) {
		std::map<FT_UInt, unsigned>::iterator g = self->glyphs.find(glyphs[i].codepoint);
		if (g == self->glyphs.end()) {
			self->cacheGlyph(glyphs[i].codepoint);
		}
	}

	self->tex->unbind();
}

void Font::draw(float x, float y, const std::string& text, const scene::Material::ptr& material)
{
	if (!self)
		throw EmptyFontException();

	hb_buffer_t* buffer = hb_buffer_create();
	on_scope_exit({ hb_buffer_destroy(buffer); });

	hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
	hb_buffer_add_utf8(buffer, text.c_str(), static_cast<int>(text.size()), 0, static_cast<int>(text.size()));
	hb_shape(self->font, buffer, NULL, 0);

	unsigned len = hb_buffer_get_length(buffer);
	hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(buffer, 0);
	hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, 0);

	glBindVertexArray(self->vao);

	material->set_parameter("color", self->color);
	material->set_parameter("texture", self->tex);
	material->set_parameter("pos", math::Vector2f{});
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	material->bind();

	self->pen_x = static_cast<int>(x);
	self->pen_y = static_cast<int>(y);
	for (unsigned i = 0; i < len; i++)
	{
		std::map<FT_UInt, unsigned>::iterator g = self->glyphs.find(glyphs[i].codepoint);
		if (g == self->glyphs.end())
		{
			glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
			g = self->cacheGlyph(glyphs[i].codepoint);
		}

		unsigned glyph = g->second;

		material->set_parameter("pos", math::Vector2f{ self->pen_x + positions[i].x_offset, self->pen_y + positions[i].y_offset });
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (GLvoid*)(glyph*GLYPH_IDX_SIZE));
		self->pen_x += positions[i].x_advance >> 6;
		self->pen_y += positions[i].y_advance >> 6;
	}

	scene::Material::unbind();
}

void Font::draw(const std::string& text, const math::Rectanglef& bounds, const math::Vector4f& color, HAlignment halign, VAlignment valign, const scene::Material::ptr& material) const
{
	/*
	Layout:
	while remaining text
	if char is newline
	add pending word to line
	start new line, start new word
	reset pos to -bbox.ymin
	else if char is space
	add pending word to line
	start new word
	add char width to pos
	else
	add char to word
	add char width to pos
	if width exceeds bounds.width
	start new line
	reset pos to -bbox.ymin
	Draw:
	if vertical align is top
	set start pos.y to ascender
	set y_inc to height + linespace
	else if vertical align is bottom
	set start pos.y to bounds.bottom - line count * height + descender
	set y_inc to height + linespace
	else if vertical align is justified
	set start pos.y to ascender
	set y_inc to max(height + linespace, (bounds.bottom - bounds.top + descender) / line count)
	for each line
	if align left or align justified
	set start pos.x to bounds.left - bbox.ymin
	if align justified
	set distributeWidth to bounds.width - line.width - bbox.ymin
	else if align right
	set start pos.x to bounds.right - line.width - bbox.ymin
	for each word
	for each glyph
	draw glyph
	if align justified and word.count is 1
	add distributeWidth / (glyph count - 1) to pos
	if align justified and word.count is not 1
	add distributeWidth / (word count - 1) to pos
	*/
	hb_buffer_t* buffer = hb_buffer_create();
	on_scope_exit({ hb_buffer_destroy(buffer); });
	hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
	hb_buffer_add_utf8(buffer, text.c_str(), static_cast<int>(text.size()), 0, static_cast<int>(text.size()));
	hb_shape(self->font, buffer, NULL, 0);
	unsigned len = hb_buffer_get_length(buffer);
	hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(buffer, 0);
	hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, 0);

	struct Word
	{
		Word(size_t glyphIdx_ = 0) : glyphIdx(glyphIdx_), len(0) {}
		void add_char(char ch, int length) { text += ch; len += length; }

		std::string text;
		size_t glyphIdx;
		int len;
	};

	struct Line
	{
		Line() : noSpacesLen(0), newLine(false) {}

		void add_word(const Word& word)
		{
			noSpacesLen += word.len;
			words.push_back(word);
		}

		int len_no_spaces() const { return noSpacesLen; }

		int len(int spaceLen) const
		{
			return words.empty() ? 0 : (len_no_spaces() + spaceLen * ((int)words.size() - 1));
		}

		int len_with_word(int spaceLen, const Word& word) const
		{
			return words.empty() ? word.len : ((noSpacesLen + word.len) + spaceLen * (int)words.size());
		}

		std::vector<Word> words;
		int noSpacesLen;
		bool newLine;
	};

	std::vector<Line> lines;

	{
		Line pendingLine;
		Word pendingWord;
		for (size_t idx = 0, textLength = text.length(); idx != textLength; ++idx)
		{
			auto ch = text[idx];
			if (ch == '\n')
			{
				if (!pendingWord.text.empty())
					pendingLine.add_word(pendingWord);
				pendingLine.newLine = true;
				lines.push_back(pendingLine);
				pendingLine = Line();
				pendingWord = Word{ idx + 1 };
			}
			else if (ch == ' ' || ch == '\t')
			{
				if (!pendingWord.text.empty())
				{
					pendingLine.add_word(pendingWord);
				}
				pendingWord = Word{ idx + 1 };
			}
			else
			{
				pendingWord.add_char(ch, positions[idx].x_advance >> 6);
				if (pendingLine.len_with_word(self->spaceWidth, pendingWord) > bounds.width())
				{
					if (!pendingLine.words.empty())
					{
						lines.push_back(pendingLine);
					}
					pendingLine = Line{};
				}
			}
		}
		if (!pendingWord.text.empty())
			pendingLine.add_word(pendingWord);
		pendingLine.newLine = true;
		lines.push_back(pendingLine);
	}

{
	glBindVertexArray(self->vao);

	material->set_parameter("color", color);
	material->set_parameter("texture", self->tex);
	material->set_parameter("pos", math::Vector2f{});
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	material->bind();

	math::Vector2f pos;
	int y_inc = self->y_size + self->lineGap;
	if (valign == VAlignment::AlignBottom)
	{
		pos.y = bounds.bottom + self->descender - (lines.size() * y_inc);
	}
	else if (valign == VAlignment::AlignCenterVert)
	{
		pos.y = bounds.top + (bounds.height() - y_inc * lines.size()) / 2 + self->ascender;
	}
	else //if ((alignment & Alignment::AlignTop) == Alignment::AlignTop)
	{
		pos.y = bounds.top + self->ascender;
	}

	for (const auto& line : lines)
	{
		float distributeWidth = 0.0f;
		bool justify = (halign == HAlignment::AlignJustified) && !line.newLine;
		if (halign == HAlignment::AlignRight)
		{
			pos.x = bounds.right - line.len(self->spaceWidth);
		}
		else if (halign == HAlignment::AlignCenterHoriz)
		{
			pos.x = bounds.left + (bounds.width() - line.len(self->spaceWidth)) / 2;
		}
		else
		{
			pos.x = bounds.left;
			if (justify)
				distributeWidth = bounds.width() - line.noSpacesLen;
		}

		for (const auto& word : line.words)
		{
			for (size_t chIdx = 0, glyphIdx = word.glyphIdx; chIdx < word.text.length(); ++chIdx, ++glyphIdx)
			{
				auto g = self->glyphs.find(glyphs[glyphIdx].codepoint);
				if (g == self->glyphs.end())
				{
					glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
					g = self->cacheGlyph(glyphs[glyphIdx].codepoint);
				}

				unsigned glyph = g->second;

				material->set_parameter("pos", pos + math::Vector2f{ positions[glyphIdx].x_offset, positions[glyphIdx].y_offset });
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (GLvoid*)(glyph*GLYPH_IDX_SIZE));
				pos.x += positions[glyphIdx].x_advance >> 6;

				if (justify && line.words.size() == 1)
				{
					pos.x += distributeWidth / (word.text.length() - 1);
				}
			}
			if (justify && line.words.size() != 1)
			{
				pos.x += distributeWidth / (line.words.size() - 1);
			}
			else
			{
				pos.x += self->spaceWidth;
			}
		}

		pos.y += y_inc;
	}
	scene::Material::unbind();
}
}

std::ostream& operator<<(std::ostream& stream, const gltext::Font::HAlignment& val)
{
	//static const std::array<std::string, 7> aligns{
	//	"AlignLeft",
	//	"AlignRight",
	//	"AlignCenterHoriz",
	//	"AlignJustified",
	//	"AlignTop",
	//	"AlignBottom",
	//	"AlignCenterVert"
	//};

	//std::stringstream ss;
	//bool first = true;
	//for (size_t idx = 0; idx < aligns.size(); ++idx)
	//{
	//	if (val & static_cast<gltext::Font::Alignment>(1 << idx))
	//	{
	//		if (!first)
	//		{
	//			ss << "|";
	//			first = true;
	//		}
	//		ss << aligns[idx];
	//	}
	//}
	//return stream << ss.str();

	switch (val)
	{
	case gltext::Font::HAlignment::AlignCenterHoriz:
		return stream << "AlignCenterHoriz";
	case gltext::Font::HAlignment::AlignJustified:
		return stream << "AlignJustified";
	case gltext::Font::HAlignment::AlignRight:
		return stream << "AlignRight";
	case gltext::Font::HAlignment::AlignLeft:
	default:
		return stream << "AlignLeft";
	};
}

std::istream& operator>>(std::istream& stream, gltext::Font::HAlignment& val)
{
	std::string str;
	stream >> str;
	boost::trim(str);
	if (str == "AlignCenterHoriz")
		val = gltext::Font::HAlignment::AlignCenterHoriz;
	else if (str == "AlignJustified")
		val = gltext::Font::HAlignment::AlignJustified;
	else if (str == "AlignRight")
		val = gltext::Font::HAlignment::AlignRight;
	else //if (str == "AlignLeft")
		val = gltext::Font::HAlignment::AlignLeft;
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const gltext::Font::VAlignment& val)
{
	switch (val)
	{
	case gltext::Font::VAlignment::AlignBottom:
		return stream << "AlignBottom";
	case gltext::Font::VAlignment::AlignCenterVert:
		return stream << "AlignCenterVert";
	case gltext::Font::VAlignment::AlignTop:
	default:
		return stream << "AlignTop";
	};
}

std::istream& operator>>(std::istream& stream, gltext::Font::VAlignment& val)
{
	std::string str;
	stream >> str;
	boost::trim(str);
	if (str == "AlignBottom")
		val = gltext::Font::VAlignment::AlignBottom;
	else if (str == "AlignCenterVert")
		val = gltext::Font::VAlignment::AlignCenterVert;
	else //if (str == "AlignTop")
		val = gltext::Font::VAlignment::AlignTop;
	return stream;
}
}
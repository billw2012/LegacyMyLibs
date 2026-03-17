#pragma once

#include "component.h"
#include "GLBase\texture.h"

namespace vice {;

struct Renderer
{
	static void static_init();
	static void static_release();

	static void render(const ComponentInstance& component);

};

}
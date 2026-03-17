
uniform sampler2DRect uBackgroundTexture;
uniform sampler2D uTexture;

in FragmentIn
{
	vec2 uv;
} In;

layout(location = 0) out vec4 out_color;

uniform vec4 uColor;

void main()
{
	// float backgroundCol = texture2D(uColourTexture, In.uv);
	out_color = uColor * texture(uTexture, In.uv);
}

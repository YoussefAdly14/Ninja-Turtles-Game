#version 330 core
// learnopengl In-Practice/Text-Rendering: the glyph bitmap (red channel) is the alpha
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec4 textColor;   // vec4 instead of the page's vec3 so callers can fade text too

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = textColor * sampled;
}

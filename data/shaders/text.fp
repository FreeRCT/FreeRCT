#version 330 core
in vec2 tex_coords;
out vec4 colour;

uniform sampler2D text;
uniform vec4 text_colour;

void main() {
	colour = vec4(text_colour.rgb, texture(text, tex_coords).r * text_colour.a);
}

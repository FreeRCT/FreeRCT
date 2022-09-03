#version 330 core
in vec2 tex_coords;
out vec4 colour;

uniform sampler2D text;
uniform vec3 text_colour;

void main() {
	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, tex_coords).r);
	colour = vec4(text_colour, 1.0) * sampled;
}

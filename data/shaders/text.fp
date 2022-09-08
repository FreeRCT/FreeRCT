#version 300 es
precision mediump float;

in vec2 tex_coords;
out vec4 colour;

uniform sampler2D text;
uniform float text_colour_r;
uniform float text_colour_g;
uniform float text_colour_b;
uniform float text_colour_a;

void main() {
	colour = vec4(text_colour_r, text_colour_g, text_colour_b, texture(text, tex_coords).r * text_colour_a);
}

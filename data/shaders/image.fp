#version 300 es
precision mediump float;

out vec4 frag_colour;

in vec4 v_overlay;
in vec2 v_texel;

uniform sampler2D tex;

void main() {
	frag_colour = texture(tex, v_texel);
	frag_colour *= v_overlay;
}

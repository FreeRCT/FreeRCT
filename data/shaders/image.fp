#version 330 core
out vec4 frag_color;
  
in vec4 v_overlay;
in vec2 v_texel;

uniform sampler2D tex;

void main() {
    frag_color = texture(tex, v_texel);
    frag_color *= v_overlay;
}


#version 300 es
precision mediump float;

out vec4 f_col;
in vec4 v_col;

void main() {
	f_col = v_col;
}

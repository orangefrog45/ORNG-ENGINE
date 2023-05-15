#version 430 core

in layout(location = 0) vec3 pos;
in layout(location = 3) mat4 transform;
uniform mat4 pv_matrix;


void main() {
	gl_Position = pv_matrix * transform * vec4(pos, 1.0);
}



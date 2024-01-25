#version 430 core

in layout(location = 0) vec3 pos;

#ifdef COLOR
in layout(location = 1) vec3 icol;
out vec3 col;
#endif

ORNG_INCLUDE "BuffersINCL.glsl"


uniform mat4 transform;
void main() {
#ifdef COLOR
	col = icol;
#endif
	gl_Position = PVMatrices.proj_view * transform * vec4(pos, 1.f);
}
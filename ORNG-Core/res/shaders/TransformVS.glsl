#version 430 core

in layout(location = 0) vec3 pos;
in layout(location = 1) vec3 normal;

#ifdef COLOR
in layout(location = 1) vec3 icol;
out vec3 col;
#endif

ORNG_INCLUDE "BuffersINCL.glsl"

#ifdef OUTLINE
uniform float u_scale;
#endif

uniform mat4 transform;
void main() {
#ifdef COLOR
	col = icol;
#endif

#ifdef OUTLINE

	float l = length(ubo_common.camera_pos.xyz - (vec3(transform[3][0], transform[3][1], transform[3][2])));
	if (u_scale < 1.01)
	gl_Position = PVMatrices.proj_view * transform * vec4(pos, 1.f);
	else
	gl_Position = PVMatrices.proj_view * transform * vec4(pos * (u_scale + max(log(l * 0.08) * 0.1, 0)), 1.f);

#else
	gl_Position = PVMatrices.proj_view * transform * vec4(pos, 1.f);
#endif
}
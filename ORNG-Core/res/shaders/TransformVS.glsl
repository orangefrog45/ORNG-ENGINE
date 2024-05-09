#version 430 core

in layout(location = 0) vec3 pos;
in layout(location = 2) vec3 normal;

#ifdef COLOR
in layout(location = 1) vec3 icol;
out vec3 col;
#endif

ORNG_INCLUDE "BuffersINCL.glsl"

#ifdef OUTLINE
uniform float u_scale;
#endif

uniform mat4 transform;

out vec3 vs_world_pos;

void main() {
#ifdef COLOR
	col = icol;
#endif

#ifdef OUTLINE
	float l = length(ubo_common.camera_pos.xyz - (vec3(transform[3][0], transform[3][1], transform[3][2])));
	if (u_scale < 1.01)
	gl_Position = PVMatrices.proj_view * transform * vec4(pos, 1.f);
	else
	gl_Position = PVMatrices.proj_view *( transform * vec4(pos, 1.f) + vec4(normal, 0) *  (u_scale * 0.1));

#else
	vs_world_pos = vec3(transform * vec4(pos, 1.f));
	gl_Position = PVMatrices.proj_view * vec4(vs_world_pos, 1.f);
#endif
}
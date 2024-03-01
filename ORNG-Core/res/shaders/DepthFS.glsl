#version 430 core

ORNG_INCLUDE "BuffersINCL.glsl"
in vec3 vs_normal;
in vec3 world_pos;


uniform bool u_alpha_test;
layout(binding = 1) uniform sampler2D albedo_sampler;
in vec2 vs_tex_coords;

#ifdef POINTLIGHT
uniform vec3 u_light_pos;
uniform float u_light_zfar;
#endif


#ifdef SPOTLIGHT
uniform vec3 u_light_pos;
#endif

void main() {
	if (u_alpha_test && texture(albedo_sampler, vs_tex_coords).a < 0.5) {
		discard;
	}
	vec3 n = normalize(vs_normal * (gl_FrontFacing ? 1.0 : -1.0));
#ifdef ORTHOGRAPHIC
	float bias = clamp(0.0005 * tan(acos(clamp(dot(n, ubo_global_lighting.directional_light.direction.xyz), 0.0, 1.0))), 0.0, 0.01); // slope bias
#elif defined SPOTLIGHT
	float bias = clamp(0.000025 * tan(acos(clamp(dot(n, normalize(u_light_pos - world_pos)), 0.0, 1.0))), 0.0, 0.01); // slope bias
#elif defined POINTLIGHT
	float bias = 0;
#endif

#ifdef POINTLIGHT
	float distance = length(world_pos - u_light_pos);
	bias = (1.0 - abs(dot(n, normalize(world_pos - u_light_pos)))) * 0.3;
	distance += bias;
	distance /= u_light_zfar;
	gl_FragDepth = distance;
#else
	gl_FragDepth = gl_FragCoord.z + bias;
#endif
}
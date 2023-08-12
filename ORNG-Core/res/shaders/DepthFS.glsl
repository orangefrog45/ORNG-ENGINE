R""(#version 430 core

in vec3 vs_normal;
in vec3 world_pos;



struct DirectionalLight {
	vec4 direction;
	vec4 color;

	//stored in vec3 instead of array due to easier alignment
	vec3 cascade_ranges;
	float size;
	float blocker_search_size;
};

layout(std140, binding = 1) uniform GlobalLighting{
	DirectionalLight directional_light;
} ubo_global_lighting;


#ifdef POINTLIGHT
uniform vec3 u_light_pos;
uniform float u_light_zfar;
#endif

void main() {
#ifdef ORTHOGRAPHIC
	float bias = clamp(0.0005 * tan(acos(clamp(dot(normalize(vs_normal), ubo_global_lighting.directional_light.direction.xyz), 0.0, 1.0))), 0.0, 0.01); // slope bias
#elif defined PERSPECTIVE
	float bias = 0;
#endif

#ifdef POINTLIGHT
	float distance = length(world_pos - u_light_pos);
	 bias = (1.0 - abs(dot(normalize(vs_normal), normalize(world_pos - u_light_pos)))) * 0.3;
distance += bias;
	distance /= u_light_zfar;
	gl_FragDepth = distance;
#else
	gl_FragDepth = gl_FragCoord.z + bias;
#endif
})""
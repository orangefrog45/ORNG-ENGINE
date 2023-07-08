#version 430 core

in vec3 vs_normal;
in vec3 world_pos;



struct DirectionalLight {
	vec4 direction;
	vec4 color;

	//stored in vec3 instead of array due to easier alignment
	vec3 cascade_ranges;
};

layout(std140, binding = 1) uniform GlobalLighting{
	DirectionalLight directional_light;
} ubo_global_lighting;


void main() {
	float bias = 0.0005 * tan(acos(dot(normalize(vs_normal), ubo_global_lighting.directional_light.direction.xyz))); // slope bias
	gl_FragDepth = gl_FragCoord.z + bias;
}
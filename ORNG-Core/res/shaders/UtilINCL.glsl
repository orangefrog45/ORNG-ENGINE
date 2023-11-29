ORNG_INCLUDE "BuffersINCL.glsl"

vec3 WorldPosFromDepth(float depth, vec2 normalized_tex_coords) {
	vec4 clipSpacePosition = vec4(normalized_tex_coords, depth, 1.0) * 2.0 - 1.0;
	vec4 viewSpacePosition = PVMatrices.inv_projection * clipSpacePosition;
	vec4 worldSpacePosition = PVMatrices.inv_view * viewSpacePosition;
	// Perspective division
	worldSpacePosition.xyz /= max(worldSpacePosition.w, 1e-6);
	return worldSpacePosition.xyz;
}


float PHI = 1.61803398874989484820459;  // Φ = Golden Ratio   

float gold_noise(in vec2 xy, in float seed){
       return fract(tan(distance(xy*PHI, xy)*seed)*xy.x);
}

float random(vec3 seed) {
	return fract(sin(dot(seed, vec3(12.9898, 78.233, 45.543))) * 43758.5453);
}

float random1(float seed)
{
	return fract(sin(seed * 43758.5453));
}









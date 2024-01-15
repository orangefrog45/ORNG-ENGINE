ORNG_INCLUDE "BuffersINCL.glsl"

#define PI 3.14159265

vec3 WorldPosFromDepth(float depth, vec2 normalized_tex_coords) {
	vec4 clipSpacePosition = vec4(normalized_tex_coords, depth, 1.0) * 2.0 - 1.0;
	vec4 viewSpacePosition = PVMatrices.inv_projection * clipSpacePosition;
	vec4 worldSpacePosition = PVMatrices.inv_view * viewSpacePosition;
	// Perspective division
	worldSpacePosition.xyz /= max(worldSpacePosition.w, 1e-6);
	return worldSpacePosition.xyz;
}


float rnd(vec2 x)
{
    int n = int(x.x * 40.0 + x.y * 6400.0);
    n = (n << 13) ^ n;
    return 1.0 - float( (n * (n * n * 15731 + 789221) + \
             1376312589) & 0x7fffffff) / 1073741824.0;
}


 vec3 qtransform( vec4 q, vec3 v ){ 
	return v + 2.0*cross(cross(v, q.xyz ) + q.w*v, q.xyz);
} 

// calculate floating point numbers equality accurately
bool isApproximatelyEqual(float a, float b)
{
	const float EPSILON = 0.00001f;
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}

// get the max value between three values
float max3(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

float max4(vec4 v)
{
    return max(max(max(v.x, v.y), v.z), v.w);
}







#version 430 core
out vec4 Fragcolour;
in vec3 vs_local_pos;

uniform layout(binding = 1) sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

void main()
{
	vec2 uv = SampleSphericalMap(normalize(vs_local_pos)); // make sure to normalize localPos
	vec3 colour = texture(equirectangularMap, uv).rgb;

	Fragcolour = vec4(colour, 1.0);
}
#version 460 core

#define PI 3.1415926538
out vec4 frag_color;
in vec3 vs_local_pos;

layout(binding = 1) uniform samplerCube environment_map_sampler;

uniform float u_roughness;
uniform float u_env_cubemap_res;

float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}


vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}


vec3 ImportanceSampleGGX(vec2 u, vec3 n, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * u.x; // Uniformly random value in range 0-2pi
	float cos_theta = sqrt((1.0 - u.y) / (1.0 + (a * a - 1.0) * u.y)); // Apply inverse transform using inverse of CDF
	float sin_theta = sqrt(1.0 - cos_theta * cos_theta);


	// Spherican to cartesian
	vec3 h;
	h.x = cos(phi) * sin_theta;
	h.y = sin(phi) * sin_theta;
	h.z = cos_theta;

	// Tangent-space to world-space
	vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0); // Ensure normal is not parallel to up vector
	vec3 tangent = normalize(cross(up, n));
	vec3 bitangent = cross(n, tangent);

	vec3 sample_vec = tangent * h.x + bitangent * h.y + n * h.z;
	return normalize(sample_vec);
}

float DistributionGGX(vec3 h, vec3 normal, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float n_dot_h = max(dot(normal, h), 0.0);
	float n_dot_h2 = n_dot_h * n_dot_h;

	float num = a2;
	float denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}


vec3 SpecularPrefilter(vec3 n) {
// viewing angle assumed to be 0
	vec3 r = n;
	vec3 v = n;

	const uint sample_count = 1024u;
	float total_weight = 0.0;
	vec3 prefiltered_color = vec3(0.0);
	
	for (uint i = 0u; i < sample_count; i++) {
		vec2 u = Hammersley(i, sample_count);
		vec3 h = ImportanceSampleGGX(u, n, u_roughness); // Bias towards normal
		vec3 l = normalize(2.0 * dot(v, h) * h - v);

		float n_dot_l = max(dot(n, l), 0.0);
		if (n_dot_l > 0.0) {
			float D = DistributionGGX(h, n, u_roughness);
			float pdf = (D * clamp(dot(n, h), 0.0, 1.0)) / (4.0 * clamp(dot(h, v), 0.0, 1.0)) + 0.0001;
			float resolution = 4096.0;
			float sa_texel = 4.0 * PI / (6.0 * resolution * resolution);
			float sa_sample = 1.0 / (float(sample_count) * pdf + 0.0001);
			float mip_level = u_roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel);

			prefiltered_color += textureLod(environment_map_sampler, l, mip_level).rgb * n_dot_l;
			total_weight += n_dot_l;
		}
	}

	prefiltered_color /= total_weight;
	return prefiltered_color;

}

void main() {
	vec3 n = normalize(vs_local_pos);
	frag_color = vec4(SpecularPrefilter(n), 1.0);

}
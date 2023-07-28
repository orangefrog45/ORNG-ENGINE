#version 460 core

#define PI 3.1415926538
out vec4 frag_color;

in vec2 tex_coords;

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


float GeometrySchlickGGX(float n_dot_v, float roughness) {
	float a = roughness;
	float k = (a * a) / 2.0;

	return n_dot_v / (n_dot_v * (1.0 - k) + k);
}

float GeometrySmith(vec3 v, vec3 l, vec3 n, float roughness) {
	float n_dot_v = max(dot(n, v), 0.0);
	float n_dot_l = max(dot(n, l), 0.0);
	float ggx2 = GeometrySchlickGGX(n_dot_v, roughness);
	float ggx1 = GeometrySchlickGGX(n_dot_l, roughness);

	return ggx2 * ggx1;
}

vec2 IntegrateBDRF(float n_dot_v, float roughness) {
	vec3 v = vec3(sqrt(1.0 - n_dot_v * n_dot_v), 0.0, n_dot_v);

	float a = 0.0;
	float b = 0.0;

	vec3 n = vec3(0.0, 0.0, 1.0);

	const uint sample_count = 1024u;

	for (uint i = 0u; i < sample_count; i++) {
		vec2 u = Hammersley(i, sample_count);
		vec3 h = ImportanceSampleGGX(u, n, roughness); // Bias towards normal
		vec3 l = normalize(2.0 * dot(v, h) * h - v);

		float n_dot_l = max(l.z, 0.0);
		float n_dot_h = max(h.z, 0.0);
		float v_dot_h = max(dot(v, h), 0.0);

		if (n_dot_l > 0.0) {
			float g = GeometrySmith(v, l, n, roughness);
			float g_vis = (g * v_dot_h) / (n_dot_h * n_dot_v);
			float fc = pow(1.0 - v_dot_h, 5.0);

			a += (1.0 - fc) * g_vis;
			b += fc * g_vis;

		}
	}

	a /= float(sample_count);
	b /= float(sample_count);

	return vec2(a, b);

}

void main() {
	frag_color = vec4(IntegrateBDRF(tex_coords.x, tex_coords.y), 1.0, 1.0);
}
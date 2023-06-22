#version 460 core

#define PI 3.1415926538
out vec4 frag_color;
in vec3 vs_local_pos;

layout(binding = 1) uniform samplerCube environment_map_sampler;


vec3 DiffusePrefilter(vec3 normal) {
	vec3 irradiance = vec3(0.0);

	vec3 up = vec3(0.0, 1.0, 0.0);


	vec3 tangent = vec3(0.0, 1.0, 0.0); // If normal and up vector are parallel, default to this
	if (dot(up, normal) > 1e-6)
		tangent = normalize(cross(up, normal));

	vec3 bitangent = normalize(cross(normal, tangent));

	mat3 tbn = mat3(tangent, bitangent, normal);

	float sample_delta = 0.025;
	uint sample_count = 0;

	for (float phi = 0.0; phi < 2.0 * PI; phi += sample_delta) {
		for (float theta = 0.0; theta < 0.5 * PI; theta += sample_delta) {

			vec3 tangent_sample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			vec3 sample_vec = tbn * tangent_sample;

			irradiance += texture(environment_map_sampler, sample_vec).rgb * cos(theta) * sin(theta);
			sample_count++;
		}
	}

	irradiance = PI * irradiance / float(sample_count);

	return irradiance;
}



void main() {
	vec3 n = normalize(vs_local_pos);
	frag_color = vec4(DiffusePrefilter(n), 1.0);
}
#version 430 core

ORNG_INCLUDE "UtilINCL.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

const unsigned int NUM_SHADOW_CASCADES = 3;
ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);


layout(binding = 1) uniform sampler2D albedo_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 16) uniform sampler2D view_depth_sampler;
layout(binding = 7) uniform sampler2D normal_sampler;
layout(binding = 11) uniform sampler2D blue_noise_sampler;
layout(binding = 12) uniform usampler2D shader_id_sampler;
layout(binding = 19) uniform sampler2D roughness_metallic_ao_sampler;
layout(binding = 20) uniform samplerCube diffuse_prefilter_sampler;
layout(binding = 21) uniform samplerCube specular_prefilter_sampler;
layout(binding = 22) uniform sampler2D brdf_lut_sampler;
layout(binding = 26) uniform samplerCubeArray pointlight_depth_sampler;
layout(binding = 1, rgba16f) writeonly uniform image2D u_output_texture;

#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler
#define SPECULAR_PREFILTER_SAMPLER specular_prefilter_sampler
#define DIFFUSE_PREFILTER_SAMPLER diffuse_prefilter_sampler
#define BRDF_LUT_SAMPLER brdf_lut_sampler

ORNG_INCLUDE "LightingINCL.glsl"

unsigned int shader_id = texelFetch(shader_id_sampler, tex_coords, 0).r;
vec3 sampled_world_pos = WorldPosFromDepth(texelFetch(view_depth_sampler, tex_coords, 0).r, tex_coords / vec2(imageSize(u_output_texture)));
vec3 sampled_normal = normalize(texelFetch(normal_sampler, tex_coords, 0).xyz);
vec3 sampled_albedo = texelFetch(albedo_sampler, tex_coords, 0).xyz;

vec3 roughness_metallic_ao = texelFetch(roughness_metallic_ao_sampler, tex_coords, 0).rgb;
float roughness = roughness_metallic_ao.r;
float metallic = roughness_metallic_ao.g;
float ao = roughness_metallic_ao.b;




void main()
{
	if (shader_id != 1) { // 1 = default lighting shader id
		imageStore(u_output_texture, tex_coords, vec4(sampled_albedo.rgb, 1.0));
		return;
	}


	vec3 total_light = vec3(0.0, 0.0, 0.0);
	vec3 v = normalize(ubo_common.camera_pos.xyz - sampled_world_pos);
	vec3 r = reflect(-v, sampled_normal);
	float n_dot_v = max(dot(sampled_normal, v), 0.0);

	//reflection amount
	vec3 f0 = vec3(0.04); // TODO: Support different values for more metallic objects
	f0 = mix(f0, sampled_albedo.xyz, metallic);

	total_light += CalculateDirectLightContribution(v, f0, sampled_world_pos.xyz, sampled_normal.xyz, roughness, metallic, sampled_albedo.rgb);
	total_light += CalculateAmbientLightContribution(n_dot_v, f0, r, roughness, sampled_normal.xyz, ao, metallic, sampled_albedo.rgb);

	vec3 light_color = max(vec3(total_light), vec3(0.0, 0.0, 0.0));

	imageStore(u_output_texture, tex_coords, vec4(light_color, 1.0));
};
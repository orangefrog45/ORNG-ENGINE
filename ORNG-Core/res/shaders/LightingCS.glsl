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
layout(binding = 27) uniform sampler3D voxel_grid_sampler;

layout(binding = 1, rgba16f) writeonly uniform image2D u_output_texture;

#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler
#define SPECULAR_PREFILTER_SAMPLER specular_prefilter_sampler
#define DIFFUSE_PREFILTER_SAMPLER diffuse_prefilter_sampler
#define BRDF_LUT_SAMPLER brdf_lut_sampler

uniform vec3 u_aligned_camera_pos;

ORNG_INCLUDE "LightingINCL.glsl"

unsigned int shader_id = texelFetch(shader_id_sampler, tex_coords, 0).r;
vec3 sampled_world_pos = WorldPosFromDepth(texelFetch(view_depth_sampler, tex_coords, 0).r, tex_coords / vec2(imageSize(u_output_texture)));
vec3 sampled_normal = normalize(texelFetch(normal_sampler, tex_coords, 0).xyz);
vec3 sampled_albedo = texelFetch(albedo_sampler, tex_coords, 0).xyz;

vec3 roughness_metallic_ao = texelFetch(roughness_metallic_ao_sampler, tex_coords, 0).rgb;
float roughness = roughness_metallic_ao.r;
float metallic = roughness_metallic_ao.g;
float ao = roughness_metallic_ao.b;

vec3 cone_dirs[6] = vec3[]
(                            vec3(0, 1, 0),
                            vec3(0, 0.5, 0.866025),
                            vec3(0.823639, 0.5, 0.267617),
                            vec3(0.509037, 0.5, -0.700629),
                            vec3(-0.509037, 0.5, -0.700629),
                            vec3(-0.823639, 0.5, 0.267617)
                            );
float cone_weights[6] = float[](0.25, 0.15, 0.15, 0.15, 0.15, 0.15);

vec3 rnd_dirs[6] =  {
	vec3(0.799,-0.377,0.470),
	vec3(-0.277,0.854,-0.440),
	vec3(-0.020,0.377,0.926),
	vec3(-0.855,0.396,-0.333),
	vec3(-0.770,-0.247,0.587),
	vec3(0.230,0.359,-0.904),
};


#define DIFFUSE_APERTURE PI / 3.0

vec3 ConeTrace(vec3 cone_dir) {
	cone_dir = normalize(cone_dir);
	vec4 col = vec4(0);

	const float tan_half_angle = tan(DIFFUSE_APERTURE * 0.5);
	const float tan_eighth_angle = tan(DIFFUSE_APERTURE * 0.125);
	float step_size_correction_factor = (1.0 + tan_eighth_angle) / (1.0 - tan_eighth_angle);
	float step_length = step_size_correction_factor * 0.34 ; // increasing makes diffuse softer

	float d = step_length;

	while (d < 25.0 && col.a < 0.9) {
		vec3 step_pos = sampled_world_pos + cone_dir * d - sampled_normal * 0.14;
		float diam = 2.0 * tan_half_angle * d;
		float mip = log2(diam / 0.2 );

		vec4 voxel = textureLod(voxel_grid_sampler, ((step_pos - u_aligned_camera_pos ) * 5.0 + vec3(128)) / (256.0), mip);
		
		if (voxel.a > 0.0) {
			float a = 1.0 - col.a;
			col.rgb += a  * voxel.rgb;
			col.a += a * voxel.a;
		}

		d += diam * 0.5;
	}

	return col.rgb;
	//return textureLod(voxel_grid_sampler, vec3(((sampled_world_pos - u_aligned_camera_pos ) * 5.0 + vec3(128)) / 256.0), 1).xyz;
}

vec3 CalculateIndirectDiffuseLighting() {
	vec3 col = vec3(0);
	float d = dot(vec3(0, 1, 0), sampled_normal);
	vec3 T = normalize((d > 0.999999) ? cross(sampled_normal, vec3(1, 0, 0)) : cross(sampled_normal, vec3(0, 1, 0)));
	vec3 B = normalize(cross(T, sampled_normal));

	vec3 dir = sampled_normal;
	col += ConeTrace(dir) * 0.25;
	dir =  0.7071f * sampled_normal +  0.7071f * T;
	col += ConeTrace(dir) * 0.15;
	dir =  0.7071f * sampled_normal +  0.7071f * (0.309f * T + 0.951f * B);
	col += ConeTrace(dir) * 0.15;
	dir =  0.7071f * sampled_normal +  0.7071f * (-0.809f * T + 0.588f * B);
	col += ConeTrace(dir) * 0.15;
	dir =  0.7071f * sampled_normal -  0.7071f * (-0.809f * T - 0.588f * B);
	col += ConeTrace(dir) * 0.15;
	dir =  0.7071f * sampled_normal -  0.7071f * (0.309f * T - 0.951f * B);
	col += ConeTrace(dir) * 0.15;

	/*for (int i = 0; i < 16; i++) {
		if (dot(diffuseConeDirections[i], sampled_normal) < -0.0 )
		continue;
		
		col += ConeTrace(diffuseConeDirections[i]);
	}*/

	return col;
}

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
	total_light += CalculateIndirectDiffuseLighting() * sampled_albedo.xyz ;

	vec3 light_color = max(vec3(total_light), vec3(0.0, 0.0, 0.0));

	imageStore(u_output_texture, tex_coords, vec4(light_color, 1.0));
};
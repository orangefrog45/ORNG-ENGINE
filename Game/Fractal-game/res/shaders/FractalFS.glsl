#version 460 core


#ifdef FULL_RES_PASS

out layout(location = 0) vec4 normal;
out layout(location = 1) vec4 albedo;
out layout(location = 2) vec4 roughness_metallic_ao;
out layout(location = 3) uint shader_id;
in vec2 tex_coords;

#else

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(binding = 0, r16f) writeonly uniform image2D i_distance_out;
vec2 tex_coords = gl_GlobalInvocationID.xy / vec2(imageSize(i_distance_out));

#endif

#ifdef FIRST_PASS
layout(binding = 16) uniform sampler2D depth_sampler;
#else
layout(binding = 1, r16f) readonly uniform image2D i_distance_in;
#endif


ORNG_INCLUDE "BuffersINCL.glsl"
ORNG_INCLUDE "UtilINCL.glsl"

#define PARTICLES_DETACHED
ORNG_INCLUDE "ParticleBuffersINCL.glsl"



/*float map(vec3 pos) {
	float dz = 1.0;
	float length2 = dot(pos, pos);
	vec3 w = pos;

	for (int i = 0; i < 4; i++) {
		dz = 8.0 * pow(length2, 3.5) * dz + 1.0;

		// extract polar coordinates
		float r = length(w);
		float b = 8.0 * acos(w.y / r);
		float a = 8.0 * atan(w.x, w.z);
		//float b = (7.0 + 1 * sin(ubo_common.time_elapsed * 0.01)) * acos(w.y / r);
		//float a = (7.0 + 1 * cos(ubo_common.time_elapsed * 0.01)) * atan(w.x, w.z);
		w = pos + pow(r, 8.0) * vec3(sin(b) * sin(a), cos(b), sin(b) * cos(a));



		length2 = dot(w, w);
		if (length2 > 256)
			break;
	}

	// distance estimator
	return 0.25 * log(length2) * sqrt(length2) / dz;
}*/


layout(std140, binding = 8) buffer FractalAnim{
	InterpolatorV3 csize_interpolator;
	InterpolatorV1 k1_interpolator;
	InterpolatorV1 k2_interpolator;
} ssbo_fractal_animator;


//vec3 CSize = vec3(0.9 + sin(ubo_common.time_elapsed * 0.0001) * 0.1, 0.9 + cos(ubo_common.time_elapsed * 0.0001) * 0.1, 1.3);
uniform vec3 CSize;
uniform float u_k_factor_1;
uniform float u_k_factor_2;
uniform uint u_num_iters;

#ifndef FULL_RES_PASS
uniform vec2 u_pixel_znear_size;
uniform vec2 u_pixel_zfar_size;
#endif



vec3 invertSphere(vec3 position, vec3 sphereCenter, float sphereRadius) {
    // Calculate the vector from the sphere center to the current position
    vec3 offset = position - sphereCenter;
    
    // Calculate the distance from the sphere center
    float distance = length(offset);
    
    // Calculate the inverted position
    vec3 invertedPosition = sphereCenter + ((sphereRadius * sphereRadius) / distance) * offset;
    
    return invertedPosition;
}

//#define K float k = max((abs(sin(ubo_common.time_elapsed * 0.0001)) * 50.0) / (r2), 0.58);

#define K float k = max(u_k_factor_1 / r2, u_k_factor_2);

vec3 Colour(vec3 p)
{
	p = p.xzy;
	float scale = 0.25;
	for (int i = 0; i < u_num_iters; i++)
	{
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		K
		p *= k;
		scale *= k;
	}
	return  clamp(mix(abs(p.xzy) * vec3(0.2, 0.001, 0.005), vec3(0.3), min(dot(p.xzy, p.yxz), 1.0)), vec3(0), vec3(1)) ;
}

vec3 rma(vec3 p)
{
	p = p.xzy;
	float scale = 0.25;
	for (int i = 0; i < u_num_iters; i++)
	{
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		K
		p *= k;
		scale *= k;
	}
	return vec3(0.5, clamp(1.0 - p.x, 0, 1), 0.1);
}

//	vec3 i_Csize = InterpolateV3(abs(sin(ubo_common.time_elapsed * 0.001)) * 4, ssbo_fractal_animator.csize_interpolator);


// Pseudo-kleinian DE
float map(vec3 p) {
	p = p.xzy;

	float scale = 0.5;
	// Move point towards limit set
	for (int i = 0; i < u_num_iters; i++)
	{
		p = 2.0 * clamp(p, -CSize, CSize ) - p;
		float r2 = dot(p, p);
		K
		p *= k;
		scale *= k;
	}

	float l = length(p.xy);
	float rxy = l - 4.0;
	float n = l * p.z ;
	rxy = max(rxy, -(n) * 0.1);
	return rxy / abs(scale);
}

//  IQ's method of calculating normals
vec3 calcNormal(in vec3 pos, float t)
{
	float precis = 0.001 * t;

	vec2 e = vec2(1.0, -1.0) * precis;
	return normalize(e.xyy * map(pos + e.xyy) +
		e.yyx * map(pos + e.yyx) +
		e.yxy * map(pos + e.yxy) +
		e.xxx * map(pos + e.xxx));
}




void main() {
	float d = 0.01;

#ifdef FIRST_PASS
	float t = d;

	// Depth texture here is depth tex from gbuffer pass, contains depth from world geometry
	vec3 world_pos = WorldPosFromDepth(texture(depth_sampler, tex_coords).r, tex_coords);
	float max_dist_2 = dot(world_pos - ubo_common.camera_pos.xyz, world_pos - ubo_common.camera_pos.xyz); 
	vec3 march_dir = normalize(world_pos - ubo_common.camera_pos.xyz);

#else
	// Image here is the actual distance rays from the lower resolution pass reached
	float t = max(imageLoad(i_distance_in, ivec2(tex_coords * imageSize(i_distance_in))).r, d);
	
	float max_dist_2 = ubo_common.cam_zfar - t;
	max_dist_2 = max_dist_2 * max_dist_2;

	vec3 march_dir = normalize(WorldPosFromDepth(1.0, tex_coords) - ubo_common.camera_pos.xyz);
#endif

	vec3 step_pos = ubo_common.camera_pos.xyz;

	for (int i = 0; i < 0; i++) {
		step_pos = ubo_common.camera_pos.xyz + march_dir * t;
		d = map(step_pos);
		t += d;
#ifdef FULL_RES_PASS
		if (d < 0.0015 * t ) {
			vec3 norm = calcNormal(step_pos, max(t, 0.1));
			normal = vec4(norm, 1.0);
			shader_id = 1;
			albedo = vec4(Colour(step_pos) , 1.0);
			roughness_metallic_ao = vec4(rma(step_pos), 1.0);

			vec4 proj = PVMatrices.proj_view * vec4(step_pos, 1.0);
			gl_FragDepth =(( proj.z / max(proj.w, 1e-7)) + 1.0) / 2.0;
			return;
		}

		if (dot(ubo_common.camera_pos.xyz - step_pos, ubo_common.camera_pos.xyz - step_pos) > max_dist_2) {
			break;
		}
#else
		vec2 current_pixel_size_worldspace = mix(u_pixel_znear_size, u_pixel_zfar_size, t / ubo_common.cam_zfar);

		if (d < current_pixel_size_worldspace.x || d < current_pixel_size_worldspace.y) {
			imageStore(i_distance_out, ivec2(gl_GlobalInvocationID.xy), vec4(t - d, 0, 0, 1));
			return;
		}

		if (dot(ubo_common.camera_pos.xyz - step_pos, ubo_common.camera_pos.xyz - step_pos) > max_dist_2) {
			imageStore(i_distance_out, ivec2(gl_GlobalInvocationID.xy), vec4(t, 0, 0, 1));
			return;
		}
#endif
	}
	
#ifdef FULL_RES_PASS
	discard;
#endif
}
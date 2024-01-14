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


vec2 grad(ivec2 z)  // replace this anything that returns a random vector
{
	// 2D to 1D  (feel free to replace by some other)
	int n = z.x + z.y * 11111;

	// Hugo Elias hash (feel free to replace by another one)
	n = (n << 13) ^ n;
	n = (n * (n * n * 15731 + 789221) + 1376312589) >> 16;

#if 0

	// simple random vectors
	return vec2(cos(float(n)), sin(float(n)));

#else

	// Perlin style vectors
	n &= 7;
	vec2 gr = vec2(n & 1, n >> 1) * 2.0 - 1.0;
	return (n >= 6) ? vec2(0.0, gr.x) :
		(n >= 4) ? vec2(gr.x, 0.0) :
		gr;
#endif
}

float noise(vec2 p)
{
	ivec2 i = ivec2(floor(p));
	vec2 f = fract(p);

	vec2 u = f * f * (3.0 - 2.0 * f); // feel free to replace by a quintic smoothstep instead

	return mix(mix(dot(grad(i + ivec2(0, 0)), f - vec2(0.0, 0.0)),
		dot(grad(i + ivec2(1, 0)), f - vec2(1.0, 0.0)), u.x),
		mix(dot(grad(i + ivec2(0, 1)), f - vec2(0.0, 1.0)),
			dot(grad(i + ivec2(1, 1)), f - vec2(1.0, 1.0)), u.x), u.y);
}

mat3 m = mat3(0.00, 0.80, 0.60,
	-0.80, 0.36, -0.48,
	-0.60, -0.48, 0.64);

float fbm(vec3 p)
{
	float f;
	f = 0.5000 * noise(p.xz); p = m * p * 2.02;
	f += 0.2500 * noise(p.xz); p = m * p * 2.03;
	f += 0.1250 * noise(p.xz); p = m * p * 2.01;
	f += 0.0625 * noise(p.xz);
	return f;
}


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

#define K float k = max((u_k_factor_1)  / (r2  ), u_k_factor_2);

vec3 Colour(vec3 p)
{
	vec3 po = p;
	p = p.xzy;
	float scale = 1.;
	for (int i = 0; i < u_num_iters; i++)
	{
		float t = dot(p, normalize(vec3(1.0, 2., 1.5)));
		p = 2.0 * clamp(p, -CSize, CSize) - p;

		float r2 = dot(p, p);
		//float r2 = dot(p, p + sin(p.z * .3) * sin(ubo_common.time_elapsed * 0.001));
		//float r2 = dot(p, p + sin(p.z * .3)); //Alternate fractal
			K

		p *= k;
		scale *= k;
	}
	//return  clamp(vec3((length(p.xz) + length(p.yz)) / max(length(p.xy), 0.00001), length(p.yz), length(p.xy)), vec3(0.01), vec3(1)) ;
	return  clamp(mix(abs(p.xzy) * vec3(0.2, 0.001, 0.005), vec3(0.3), min(dot(p.xzy, p.yxz), 1.0)), vec3(0), vec3(1)) ;
	//return  clamp(mix(abs(p.xzy * p.zxz) * vec3(0.88, 5.6, 0.12), vec3(0.3), min(dot(p.xzy, p.yxz), 1.0)), vec3(0), vec3(1));
}

	//	float k = max((2.) / (r2), .027);
vec3 rma(vec3 p)
{
	p = p.xzy;
	float scale = 1.;
	for (int i = 0; i < u_num_iters; i++)
	{
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p, p + sin(p.z * .3) * sin(ubo_common.time_elapsed * 0.001));
		//float r2 = dot(p, p + sin(p.z * .3)); //Alternate fractal
				K

		p *= k;
		scale *= k;
	}
	return  vec3(0.5, clamp(1.0 - p.x, 0, 1), 0.1);
}

	vec3 i_Csize = InterpolateV3(abs(sin(ubo_common.time_elapsed * 0.001)) * 4, ssbo_fractal_animator.csize_interpolator);


// Pseudo-kleinian DE

float map(vec3 p) {
	p = p.xzy;

	float scale = 0.25;
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

/*float map(vec3 p) {
	//p.y += 40.0 + 20.0 * sin(ubo_common.time_elapsed * 0.0001 * p.x * 0.01);
	p = p * 0.01;
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		p = invertSphere(p, p.xzy, 1.0);

    vec4 q = vec4(p - 1.0, 1);
    for(int i = 0; i < 12; i++) {
        q.xyz = abs(q.xyz + 1.0) - 1.0;
        q /= clamp(dot(q.xyz, q.xyz), 0.25, 1.00);
        q *= 1.15;
    }
    return (length(q.zy) - 1.2)/q.w * 100.0;

}*/


vec3 calcNormal(in vec3 pos, float t)
{
	float precis = 0.001 * t;

	vec2 e = vec2(1.0, -1.0) * precis;
	return normalize(e.xyy * map(pos + e.xyy) +
		e.yyx * map(pos + e.yyx) +
		e.yxy * map(pos + e.yxy) +
		e.xxx * map(pos + e.xxx));
}


float tube(vec3 position, float innerRadius, float outerRadius, float halfHeight, float cornerRadius) {
	vec2 d = vec2(length(position.xz) - (outerRadius + innerRadius) * 0.5, position.y);
	d = abs(d) - vec2((outerRadius - innerRadius) * 0.5, halfHeight) + cornerRadius;
	return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - cornerRadius;
}

float MapVein(vec3 pos, float cyl_dist, float t) {
	float perlin = abs(noise((pos.xz / 5.0) + normalize(pos.zy) * sin(ubo_common.time_elapsed * 0.0001) * noise(pos.zy) * 0.5));
	float perlin_thickness = 0.005 * log(t) / clamp(log(cyl_dist * 0.1), 1.0, 3.0);
	return perlin - perlin_thickness;
}



void main() {
float d = 0.1;

#ifdef FIRST_PASS
	float t = d;

	// Depth texture here is depth tex from gbuffer pass, contains depth from world geometry
	vec3 world_pos = WorldPosFromDepth(texture(depth_sampler, tex_coords).r, tex_coords);
	float max_dist_2 = dot(world_pos - ubo_common.camera_pos.xyz, world_pos - ubo_common.camera_pos.xyz); 
	vec3 march_dir = normalize(world_pos - ubo_common.camera_pos.xyz);

#else
	// Image here is the actual distance rays from the lower resolution pass reached
	float t = max(imageLoad(i_distance_in, ivec2(tex_coords * imageSize(i_distance_in))).r, 0.00001);
	float max_dist_2 = ubo_common.cam_zfar - t;
	max_dist_2 = max_dist_2 * max_dist_2;

	vec3 march_dir = normalize(WorldPosFromDepth(1.0, tex_coords) - ubo_common.camera_pos.xyz);
#endif

	vec3 step_pos = ubo_common.camera_pos.xyz + march_dir * t;

	for (int i = 0; i < 196; i++) {
		d = map(step_pos);
		step_pos += d * march_dir;
		t += d;

#ifdef FULL_RES_PASS
		if (d < 0.0015 * t ) {
			vec3 norm = calcNormal(step_pos, t);
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
			imageStore(i_distance_out, ivec2(gl_GlobalInvocationID.xy), vec4(t - d , 0, 0, 1));
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
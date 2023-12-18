#version 460 core

layout(binding = 16) uniform sampler2D depth_sampler;

out layout(location = 0) vec4 normal;
out layout(location = 1) vec4 albedo;
out layout(location = 2) vec4 roughness_metallic_ao;
out layout(location = 3) uint shader_id;

in vec2 tex_coords;


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

//vec3 CSize = vec3(0.9 + sin(ubo_common.time_elapsed * 0.0001) * 0.1, 0.9 + cos(ubo_common.time_elapsed * 0.0001) * 0.1, 1.3);
vec3 CSize = vec3(0.5, 0.8, 0.9) * 1.35;

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

#define K float k = max(1.5 / (r2 ), 0.68);

vec3 Colour(vec3 p)
{
	p = p.xzy;
	float scale = 1.;
	for (int i = 0; i < 24; i++)
	{
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p, p + sin(p.z * .3) * sin(ubo_common.time_elapsed * 0.001));
		//float r2 = dot(p, p + sin(p.z * .3)); //Alternate fractal
			K

		p *= k;
		scale *= k;
	}
	return  clamp(mix(abs(p.xzy) * vec3(0.2, 0.001, 0.005), vec3(0.3), min(dot(p.xzy, p.yxz), 1.0)), vec3(0), vec3(1));
}

	//	float k = max((2.) / (r2), .027);
vec3 rma(vec3 p)
{
	p = p.xzy;
	float scale = 1.;
	for (int i = 0; i < 24; i++)
	{
		
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p, p + sin(p.z * .3) * sin(ubo_common.time_elapsed * 0.001));
		//float r2 = dot(p, p + sin(p.z * .3)); //Alternate fractal
				K

		p *= k;
		scale *= k;
	}
	return  vec3(0.5, clamp(1.0 - p.z, 0, 1), 0.01);
}



// Pseudo-kleinian DE with parameters from here https://www.shadertoy.com/view/4s3GW2
float map(vec3 p) {
	//p.y += 40.0 + 20.0 * sin(ubo_common.time_elapsed * 0.0001 * p.x * 0.01);

	vec3 o = p;
	p = p.xzy;
	float scale = 1.;
	// Move point towards limit set
	for (int i = 0; i < 12; i++)
	{
		// box fold
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p, p + p); //Alternate fractal
		K
		p *= k;
		scale *= k;
	}

	float l = length(p.xy);
	float rxy = l - 4.0;
	float n = l * p.z;
	rxy = max(rxy, -(n) / 4.);
	return (rxy) / abs(scale);

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

in vec4 gl_FragCoord;
void main() {
	float depth = texture(depth_sampler, tex_coords).r;
	vec3 world_pos = WorldPosFromDepth(depth, tex_coords);
	vec3 march_dir = normalize(world_pos - ubo_common.camera_pos.xyz);
	vec3 step_pos = ubo_common.camera_pos.xyz;

	float max_dist = length(world_pos - ubo_common.camera_pos.xyz);
	float t = 0.01;
	float d = 0.01;
	for (int i = 0; i < 256; i++) {
		d = map(step_pos);
		 step_pos += d * march_dir;
		t += d;
		if (d < 0.001 * t) {
			
			uint x = uint(gl_FragCoord.x / 1.25);
			uint y = uint(gl_FragCoord.y * ubo_common.render_resolution_x);

			uint particle_index = uint((float(x + y) / float(ubo_common.render_resolution_x * ubo_common.render_resolution_y)) * 10000.0);


			//ssbo_particles_detached.particles[particle_index].velocity_life -= ubo_common.delta_time;
			//ssbo_particles_detached.particles[particle_index].pos.xyz += ssbo_particles_detached.particles[particle_index].velocity_life.xyz * ubo_common.delta_time;


			vec3 norm = calcNormal(step_pos, t);
			normal = vec4(norm, 1.0);
			shader_id = 1;

			albedo = vec4(Colour(step_pos) + vec3(1.0, 0.2, 0.05) * exp((-i / 256.0 ) * 10.0), 1.0);

			if ( false && atomicAdd(ssbo_particles_detached.particles[particle_index].flags, 1) == 0 && ssbo_particles_detached.particles[particle_index].velocity_life.w < 0) {
				ssbo_particles_detached.particles[particle_index].velocity_life.w = 0.1;
				ssbo_particles_detached.particles[particle_index].velocity_life.xyz = normal.xyz;
				ssbo_particles_detached.particles[particle_index].scale.xyz = vec3(0.025);

				ssbo_particles_detached.particles[particle_index].scale.w = albedo.r * 10.0;
				ssbo_particles_detached.particles[particle_index].pos.w = albedo.g * 10.0;
				ssbo_particles_detached.particles[particle_index].quat.w = albedo.b * 10.0;




				ssbo_particles_detached.particles[particle_index].pos.xyz = step_pos;
			}
			//albedo = vec4(Colour(step_pos) * 0.1 * float(perlin >= 0.01) + vec3(0.6, 0.00, 0.00) * float(perlin <= 0.01) * 10.0 / clamp(cyl_dist, 0.1, 20.0), 1.0);

			roughness_metallic_ao =vec4(rma(step_pos), 1.0);
			//roughness_metallic_ao = vec4(0.5, 0.0 + float(perlin <= 0.01) * 0.8, 0.02, 1.0);

			vec4 proj = PVMatrices.proj_view * vec4(step_pos, 1.0);
			gl_FragDepth =(( proj.z / proj.w) + 1.0) / 2.0;
			return;
		}

		if (length(ubo_common.camera_pos.xyz - step_pos) > max_dist) {
			break;
		}
	}
	discard;
}
#version 460 core

layout(binding = 16) uniform sampler2D depth_sampler;

out layout(location = 0) vec4 normal;
out layout(location = 1) vec4 albedo;
out layout(location = 2) vec4 roughness_metallic_ao;
out layout(location = 3) uint shader_id;

in vec2 tex_coords;


ORNG_INCLUDE "BuffersINCL.glsl"
ORNG_INCLUDE "UtilINCL.glsl"



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
vec3 CSize = vec3(1.0, 1.0, 1.3);

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

vec3 Colour(vec3 p)
{
	p = p.xzy;
	float scale = 1.;
	for (int i = 0; i < 12; i++)
	{
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p, p + sin(p.z * .3) * sin(ubo_common.time_elapsed * 0.001));
		//float r2 = dot(p, p + sin(p.z * .3)); //Alternate fractal
		float k = max((2.) / (r2), .027);
		p *= k;
		scale *= k;
	}
	return  clamp(mix(abs(p.xzy) * vec3(0.2, 0.001, 0.005), vec3(0.3), min(dot(p.xzy, p.yxz), 1.0)), vec3(0), vec3(1));
}

vec3 rma(vec3 p)
{
	p = p.xzy;
	float scale = 1.;
	for (int i = 0; i < 12; i++)
	{
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p, p + sin(p.z * .3) * sin(ubo_common.time_elapsed * 0.001));
		//float r2 = dot(p, p + sin(p.z * .3)); //Alternate fractal
		float k = max((2.) / (r2), .027);
		p *= k;
		scale *= k;
	}
	return  vec3(0.5, clamp(1.0 - p.z, 0, 1), 0.01);
}


// Pseudo-kleinian DE with parameters from here https://www.shadertoy.com/view/4s3GW2
float map(vec3 p) {
	p = p.xzy;
	float scale = 1.;
	// Move point towards limit set
	for (int i = 0; i < 12; i++)
	{
		// box fold
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p, p + sin(p.z * .3)); //Alternate fractal
		float k = max((2.) / (r2), .027);
		p *= k;
		scale *= k;
	}
	float l = length(p.xy);
	float rxy = l - 4.0;
	float n = l * p.z;
	rxy = max(rxy, -(n) / 4.);
	return (rxy) / abs(scale);
}


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
			vec3 norm = calcNormal(step_pos, t);
			normal = vec4(norm, 1.0);
			shader_id = 1;

			float rad = int(ubo_common.time_elapsed) % 10000 * 0.1;
			float cyl_dist = tube(step_pos + step_pos * abs(noise(step_pos.xz / 15.0)), rad, rad + 1.0, 10000, 50.0);
			float perlin = MapVein(step_pos, cyl_dist, t);

			albedo = vec4(Colour(step_pos) * 0.1, 1.0);
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